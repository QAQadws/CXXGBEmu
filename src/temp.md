### 1
下面给你一套基于 standalone Asio 的最小联机实现（同一局域网/Wi‑Fi 下 TCP 一字节锁步），以及把它接入到 SERIAL 的改动。思路是“字节级锁步”：在 SC.7 上升沿锁存 SB；主机立即开始本轮传输并把字节发给对端；从机也把自己的字节发给对端，但等待收到主机字节后再开始并立即完成这一轮。未连接或断开时，主机收到 0xFF，外部时钟（从机）则会等待直到收到主机字节。

新增文件：ILink 接口与 TCP 实现
````cpp
#pragma once
#include <cstdint>

struct ILink {
    virtual ~ILink() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool connected() const = 0;

    // 非阻塞尝试弹出一个收到的字节；有则写入out并返回true
    virtual bool try_recv(uint8_t& out) = 0;

    // 异步发送一个字节（若未连接则忽略）
    virtual void send(uint8_t b) = 0;
};
````

````cpp
#pragma once
#define ASIO_STANDALONE
#include <asio.hpp>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include "link.h"

class TcpLink final : public ILink {
public:
    // server: server=true, host无视，仅需port
    // client: server=false, host="192.168.x.x", port同服务端
    TcpLink(bool server, const std::string& host, uint16_t port)
        : server_(server), host_(host), port_(port),
          socket_(ioc_) {}

    void start() override;
    void stop() override;
    bool connected() const override { return connected_; }
    bool try_recv(uint8_t& out) override;
    void send(uint8_t b) override;

private:
    void run_();
    void listen_();
    void do_connect_();
    void start_read_();
    void close_();

    bool server_;
    std::string host_;
    uint16_t port_;

    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};

    asio::io_context ioc_;
    std::unique_ptr<asio::ip::tcp::acceptor> acceptor_;
    asio::ip::tcp::socket socket_;
    std::thread th_;

    std::mutex rx_mtx_;
    std::queue<uint8_t> rx_q_;
};
````

````cpp
#include "tcp_link.h"
#include <iostream>

void TcpLink::start() {
    if (running_) return;
    running_ = true;
    th_ = std::thread([this]{ run_(); });
}
void TcpLink::stop() {
    running_ = false;
    asio::post(ioc_, [this]{ close_(); });
    if (th_.joinable()) th_.join();
}

bool TcpLink::try_recv(uint8_t& out) {
    std::lock_guard<std::mutex> lock(rx_mtx_);
    if (rx_q_.empty()) return false;
    out = rx_q_.front();
    rx_q_.pop();
    return true;
}

void TcpLink::send(uint8_t b) {
    if (!connected_) return;
    auto buf = std::make_shared<std::array<uint8_t,1>>();
    (*buf)[0] = b;
    asio::post(ioc_, [this, buf]{
        if (!connected_) return;
        asio::async_write(socket_, asio::buffer(*buf),
            [buf](std::error_code /*ec*/, std::size_t /*n*/){});
    });
}

void TcpLink::run_() {
    if (server_) listen_();
    else do_connect_();

    while (running_) {
        try {
            ioc_.run_for(std::chrono::milliseconds(50));
        } catch (...) {
            // 保活
        }
    }
    close_();
}

void TcpLink::listen_() {
    using asio::ip::tcp;
    acceptor_ = std::make_unique<tcp::acceptor>(ioc_, tcp::endpoint(tcp::v4(), port_));
    acceptor_->async_accept([this](std::error_code ec, asio::ip::tcp::socket s){
        if (ec) {
            std::cerr << "[LINK] accept error: " << ec.message() << "\n";
            return;
        }
        socket_ = std::move(s);
        connected_ = true;
        start_read_();
    });
}

void TcpLink::do_connect_() {
    using asio::ip::tcp;
    tcp::resolver res(ioc_);
    auto eps = res.resolve(host_, std::to_string(port_));
    asio::async_connect(socket_, eps, [this](std::error_code ec, const tcp::endpoint&){
        if (ec) {
            std::cerr << "[LINK] connect error: " << ec.message() << "\n";
            return;
        }
        connected_ = true;
        start_read_();
    });
}

void TcpLink::start_read_() {
    auto buf = std::make_shared<std::array<uint8_t,1>>();
    socket_.async_read_some(asio::buffer(*buf),
        [this, buf](std::error_code ec, std::size_t n){
            if (!ec && n == 1) {
                {
                    std::lock_guard<std::mutex> lock(rx_mtx_);
                    rx_q_.push((*buf)[0]);
                }
                start_read_();
            } else {
                if (ec) std::cerr << "[LINK] read error: " << ec.message() << "\n";
                connected_ = false;
                close_();
            }
        });
}

void TcpLink::close_() {
    std::error_code ec;
    if (socket_.is_open()) socket_.close(ec);
    if (acceptor_ && acceptor_->is_open()) acceptor_->close(ec);
    connected_ = false;
}
````

修改 SERIAL 接入联机（上升沿锁存 + 主从起始 + 未连接容错）
````cpp
#pragma once
#include <cstdint>
#include <vector>

struct EMU;
struct ILink; // 前向声明

using u8 = uint8_t;
using u16 = uint16_t;

class SERIAL {
public:
    void init();
    void tick(EMU* emu);

    u8 bus_read(u16 address);
    void bus_write(u16 address, u8 data);

    // 外部注入/解绑网络
    void attach_link(ILink* link) { link_ = link; }
    void detach_link() { link_ = nullptr; }

    // 辅助判断
    inline bool is_master() const { return (sc_ & 0x01) != 0; }
    inline bool transfer_enable() const { return (sc_ & 0x80) != 0; }

private:
    void begin_transfer();
    void end_transfer(EMU* emu);

    // 状态
    u8 sb_{0xFF};
    u8 sc_{0x7C};

    // 本轮传输缓存
    u8 sb_latched_{0xFF};  // 上升沿锁存的发送字节
    u8 rx_latched_{0xFF};  // 收到的对端字节
    bool have_rx_{false};  // 是否收到对端字节

    bool transferring_{false};
    int  transfer_bit_{0};
    u8   out_byte_{0};

    // 仅用于本地观测/调试
    std::vector<u8> output_buffer_;

    // 联机
    ILink* link_{nullptr};
};
````

````cpp
#include "serial.h"
#include "emu.h"
#include <iostream>
#include <cstdlib>
#include "net/link.h"

void SERIAL::init()
{
    sb_ = 0xFF;
    sc_ = 0x7C; // 0111 1100: 仅bit7/bit1/bit0有意义，其它读作1
    transferring_ = false;
    have_rx_ = false;
    sb_latched_ = 0xFF;
    rx_latched_ = 0xFF;
}

void SERIAL::tick(EMU* emu)
{
    // 网络收包：把对端发来的本轮字节取走
    if (link_) {
        u8 b;
        while (link_->try_recv(b)) {
            rx_latched_ = b;
            have_rx_ = true;
        }
    }

    // 主机（内部时钟）：若SC.7已经置1且还没开始，则立即开始
    if (!transferring_ && is_master() && transfer_enable()) {
        begin_transfer();
    }

    // 处理传输完成条件（字节级锁步简化）
    if (transferring_) {
        if (link_ && link_->connected()) {
            if (is_master()) {
                // 主机：等到拿到对端字节即可完成
                if (have_rx_) {
                    end_transfer(emu);
                }
            } else {
                // 从机：只有在收到主机字节后才开始；开始后立即完成
                if (have_rx_) {
                    // 确保已经被SC.7上升沿触发（transfer_enable为真），再开始并结束
                    if (!transferring_) begin_transfer();
                    end_transfer(emu);
                }
            }
        } else {
            // 未连接或断开
            if (is_master()) {
                // 断线场景主机读到 0xFF
                rx_latched_ = 0xFF;
                have_rx_ = true;
                end_transfer(emu);
            } else {
                // 外部时钟下应当等待时钟（不超时完成）
                // 可选：加一个超时以避免永不返回（这里选择严格等待）
            }
        }
    }
}

void SERIAL::begin_transfer()
{
    transfer_bit_ = 7;
    out_byte_ = sb_latched_; // 只用锁存字节
    transferring_ = true;
}

void SERIAL::end_transfer(EMU* emu)
{
    // 完成一字节交换：SB 读出为对端发来的字节
    sb_ = have_rx_ ? rx_latched_ : 0xFF;

    // 输出缓冲仅用于观测本端发出的字节
    output_buffer_.push_back(out_byte_);

    transferring_ = false;
    have_rx_ = false;
    sc_ &= 0x7F;                  // 清传输使能位
    emu->int_flags |= INT_SERIAL; // 触发串口中断
}

u8 SERIAL::bus_read(u16 address)
{
    if (address == 0xFF01) {
        return sb_;
    } else if (address == 0xFF02) {
        return sc_;
    }
    std::cerr << "[SERIAL] warn: Invalid read at 0x" << std::hex << address << std::dec << "\n";
    return 0xFF;
}

void SERIAL::bus_write(u16 address, u8 data)
{
    if (address == 0xFF01) {
        sb_ = data;
        return;
    } else if (address == 0xFF02) {
        // 仅允许写 bit7(启动) 与 bit0(主从), bit1 为倍速（CGB）；其余读作1
        u8 old = sc_;
        sc_ = 0x7C | (data & 0x83);

        bool old_en = (old & 0x80) != 0;
        bool new_en = (sc_ & 0x80) != 0;
        bool rising = (!old_en && new_en);

        if (rising) {
            // 在上升沿锁存本轮要发送的字节
            sb_latched_ = sb_;
            rx_latched_ = 0xFF;
            have_rx_ = false;

            // 把本轮字节发给对端
            if (link_ && link_->connected()) {
                link_->send(sb_latched_);
            }

            // 若是主机，立即进入传输态；从机等待收到主机字节再开始
            if (is_master()) {
                begin_transfer();
            } else {
                // 从机：保持transfer_enable为1，等待主机字节到来
                transferring_ = true; // 标记“本轮正在等待外部时钟”
            }
        }
        return;
    }

    std::cerr << "[SERIAL] warn: Invalid write at 0x" << std::hex << address << std::dec << "\n";
}
````

最小使用示例（在你的入口初始化里创建并注入链接）
````cpp
// ...existing code...
// 建立服务端（等待连接）
auto link_server = std::make_unique<TcpLink>(true, "", 8765);
link_server->start();
emu1.serial.attach_link(link_server.get());

// 建立客户端（连接到服务端机器IP）
auto link_client = std::make_unique<TcpLink>(false, "192.168.1.23", 8765);
link_client->start();
emu2.serial.attach_link(link_client.get());
// ...existing code...
````

构建与运行提示
- 依赖：standalone Asio（仅头文件）。Windows 需链接 Ws2_32.lib；用 CMake 可加 target_link_libraries(... ws2_32).
- 获取 Asio：
  - vcpkg: vcpkg install asio:x64-windows，然后包含 <asio.hpp>
  - 或直接把 asio 源码放到第三方目录并加入包含路径
- 运行：
  - 服务端机器启动“服务端版本”（server=true，端口同上）
  - 客户端机器（同一 Wi‑Fi）填服务端局域网 IP（如 192.168.x.x）连接
- 调试：
  - 断开时主机会读到 0xFF（符合文档 Disconnects）
  - 从机在外部时钟下会等待直到收到主机字节（可选再加超时）

说明
- 该实现先以“字节级锁步”跑通联机，避免位级时钟对网络抖动敏感的问题，符合文档对超时/断线的语义。
- 后续若要逼近 8192Hz 位移位，可在 tick 中按 CPU 周期推进 512 周期/位、8 位后完成；当前版本在收到对端字节后直接完成一轮。


### 2
下面用入门口吻把这份 tcp_link.cpp 从上到下讲清楚，并指出一处容易踩坑的细节，再给出小修正。

它在做什么
- 目的：在两台机器之间用 TCP 传输“每次 1 字节”的数据，供串口联机使用。
- 线程模型：
  - 启动后新开一个“网络线程”（th_），这个线程里运行 Asio 的事件循环 io_context。
  - 所有网络 I/O（连接、读、写、关闭）都在这个线程异步进行。
  - 仿真主线程通过 try_recv 非阻塞地“捞取”已收到的字节，通过 send 异步发送字节。
- 角色：
  - 服务器 server_=true：listen_ 打开端口等待连接。
  - 客户端 server_=false：do_connect_ 连接到 host:port。

逐段解释
- start/stop
  - start：若未运行，置 running_=true，启动线程执行 run_。
  - stop：置 running_=false，向 io_context 投递 close_（尽量在网络线程中收尾），最后 join 等线程退出。
- try_recv
  - 从接收队列 rx_q_ 里“非阻塞”地取一个字节。用互斥锁保护队列，避免与网络线程同时读写造成数据竞争。
  - 没有数据时立即返回 false，不会卡住主线程。
- send
  - 发送 1 字节。先快速判断 connected_，再把真实的 async_write 投递到 io_context 中执行（“异步”）。
  - 用 std::shared_ptr 持有缓冲区，确保在异步回调结束前内存不会被释放。
- run_
  - 根据角色调用 listen_ 或 do_connect_ 启动连接/监听。
  - 在 while(running_) 循环里，每次运行 io_context 50ms，处理期间产生的所有异步事件。
  - 出错 catch(...) 被吞掉以“保活”。更好做法是打印日志，便于定位问题（下文提供修正）。
- listen_
  - 服务器端：在 port_ 上建 acceptor 监听，async_accept 等待一个入站连接，成功后把对方的 socket 存入 socket_，标记 connected_=true，然后 start_read_ 开始收数据。
  - 注意：当前代码只 accept 一次；断开后不会自动继续 accept 新连接，除非你重新调用 listen_ 或重启。
- do_connect_
  - 客户端：解析 host:port，async_connect 进行连接，成功后标记 connected_=true 并 start_read_。
- start_read_
  - 安排一次“读 1 字节”的异步读。读到后把字节 push 到 rx_q_，然后递归调用 start_read_ 继续下一次读取。
  - 读/连接错误会打印日志，置 connected_=false，并调用 close_ 清理。
  - 共享指针 buf 保证缓冲区活到回调；回调里锁住队列后 push。
- close_
  - 带 error_code 的 close，避免抛异常。某些 Asio 版本 close(ec) 还有 [[nodiscard]] 返回值，代码用 (void)rc 吞掉返回值来消除告警。
  - 分别尝试关闭 socket 和 acceptor，打印关闭错误，最后 connected_=false。

涉及的并发与锁
- rx_q_ 队列有一个生产者（网络线程）和一个消费者（仿真线程）；用 std::mutex 简单互斥保护，防止并发访问。
- running_、connected_ 用 std::atomic<bool> 表示跨线程状态标志，避免数据竞争。
- send 的两个“connected_”检查：一个在调用线程（快路径），一个在投递后的网络线程（防止刚投递时状态变化）。

重要细节与建议改进
- 读取 1 字节的方式
  - 现在用 async_read_some，n 可能为 0、1、2…；你的处理逻辑仅接受 n==1，否则就走错误分支并 close_。
  - 这会在“对方一次发了多个字节，内核合并了读”时误判为错误。
  - 更稳妥：使用 async_read（会把缓冲区填满才返回）或 transfer_exactly(1)。这样回调里 n 一定是 1。
- 异常处理
  - 建议不要完全吞掉异常，至少打日志，便于排查。
- 发送时的竞态
  - 已在 lambda 内再次检查 connected_，可再加 socket_.is_open() 保险。
- 服务器多次接入
  - 目前服务器只接受一次连接。若希望“断开后允许重新连接”，需要在断开时再调用 listen_，或者一旦 accept 成功后就再次 post 一个新的 async_accept。
- 关闭逻辑
  - close_ 同时关 socket 和 acceptor。若你想“仅断开当前连接但继续作为服务器等待下一个连接”，不要关闭 acceptor，只关 socket 并重新 listen_。

建议的小修正
- 改用 async_read，确保一次只读 1 字节。
- 捕获并记录异常。
- 发送时再检查 socket_.is_open()。

````cpp
// ...existing code...
void TcpLink::run_() {
  if (server_)
    listen_();
  else
    do_connect_();

  while (running_) {
    try {
      ioc_.run_for(std::chrono::milliseconds(50));
    } catch (const std::exception& e) {
      std::cerr << "[LINK] io exception: " << e.what() << "\n";
    } catch (...) {
      std::cerr << "[LINK] io unknown exception\n";
    }
  }
  close_();
}
// ...existing code...
void TcpLink::send(uint8_t b) {
  if (!connected_)
    return;
  auto buf = std::make_shared<std::array<uint8_t, 1>>();
  (*buf)[0] = b;
  asio::post(ioc_, [this, buf] {
    if (!connected_ || !socket_.is_open())
      return;
    asio::async_write(socket_, asio::buffer(*buf),
                      [buf](std::error_code /*ec*/, std::size_t /*n*/) {});
  });
}
// ...existing code...
void TcpLink::start_read_() {
  auto buf = std::make_shared<std::array<uint8_t, 1>>();
  // 用 async_read 确保恰好读取 1 字节才回调
  asio::async_read(
      socket_, asio::buffer(*buf),
      [this, buf](std::error_code ec, std::size_t n) {
        if (!ec && n == 1) {
          {
            std::lock_guard<std::mutex> lock(rx_mtx_);
            rx_q_.push((*buf)[0]);
          }
          start_read_(); // 继续下一字节
        } else {
          if (ec)
            std::cerr << "[LINK] read error: " << ec.message() << "\n";
          connected_ = false;
          close_();
        }
      });
}
// ...existing code...
````

顺带回顾几个关键词
- 异步：提交 I/O 后立刻返回，真正的 I/O 在 io_context 的线程里完成，完成后以回调通知。
- 非阻塞：调用处不等待结果（比如 try_recv 立刻返回 false），不会卡住主线程。
- 互斥锁（std::mutex）：保护共享数据（rx_q_）免受多线程同时访问破坏。
- 原子变量（std::atomic<bool>）：在多线程间读写简单标志，不需要锁，避免数据竞争。

如需支持“服务端断线后继续接受下一次连接”，可把 close_ 拆分成“关 socket 不关 acceptor”，并在断线时调用 listen_ 再次 async_accept。需要我可以给出那部分改动。