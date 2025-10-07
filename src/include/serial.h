#pragma once
#include <cstdint>
#include <deque>
class EMU;
struct ILink; // 前向声明

using u8 = uint8_t;
using u16 = uint16_t;

class SERIAL {
public:
  void init();
  void tick(EMU *emu);

  u8 bus_read(u16 address);
  void bus_write(u16 address, u8 data);

  // 外部注入/解绑网络
  void attach_link(ILink *link) { link_ = link; }
  void detach_link() { link_ = nullptr; }

  // 辅助判断
  inline bool is_master() const { return (sc_ & 0x01) != 0; }
  inline bool transfer_enable() const { return (sc_ & 0x80) != 0; }

  // 仅用于本地观测/调试
  std::deque<u8> output_buffer_;
private:
  void begin_transfer();
  void end_transfer(EMU *emu);

  // 状态
  u8 sb_{0xFF};
  u8 sc_{0x7C};

  // 本轮传输缓存
  u8 sb_latched_{0xFF}; // 上升沿锁存的发送字节
  u8 rx_latched_{0xFF}; // 收到的对端字节
  bool have_rx_{false}; // 是否收到对端字节

  bool transferring_{false};
  int transfer_bit_{0};
  u8 out_byte_{0};


  // 联机
  ILink *link_{nullptr};
};


// #ifndef __SERIAL_H__
// #define __SERIAL_H__
// #include"defs.h"
// #include<deque>
// class EMU; // 前向声明
// class SERIAL {
// private:
//   u8 sb_;             // 发送缓冲区
//   bool transferring_; // 是否正在传输数据
//   u8 out_byte_;
//   s8 transfer_bit_;
  
//   public:
//   u8 sc_;             // 控制寄存器
//   std::deque<u8> output_buffer_;
//   void init();
//   void tick(EMU* emu);

//     void begin_transfer();
//     void process_transfer(EMU* emu);
//     void end_transfer(EMU* emu);

//     bool is_master() const { return (sc_ & 0x01); }
//     bool transfer_enable() const { return (sc_ & 0x80); }
//     void bus_write(u16 address, u8 data);
//     u8 bus_read(u16 address);
// };

// #endif // __SERIAL_H__