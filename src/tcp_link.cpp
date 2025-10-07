#include "tcp_link.h"
#include <iostream>

void TcpLink::start() {
  if (running_)
    return;
  running_ = true;
  th_ = std::thread([this] { run_(); });
}
void TcpLink::stop() {
  running_ = false;
  asio::post(ioc_, [this] { close_(); });
  if (th_.joinable())
    th_.join();
}

bool TcpLink::try_recv(uint8_t &out) {
  std::lock_guard<std::mutex> lock(rx_mtx_);
  if (rx_q_.empty())
    return false;
  out = rx_q_.front();
  rx_q_.pop();
  return true;
}

void TcpLink::send(uint8_t b) {
  if (!connected_)
    return;
  auto buf = std::make_shared<std::array<uint8_t, 1>>();
  (*buf)[0] = b;
  asio::post(ioc_, [this, buf] {
    if (!connected_)
      return;
    asio::async_write(socket_, asio::buffer(*buf),
                      [buf](std::error_code /*ec*/, std::size_t /*n*/) {});
  });
}

void TcpLink::run_() {
  if (server_)
    listen_();
  else
    do_connect_();

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
  acceptor_ =
      std::make_unique<tcp::acceptor>(ioc_, tcp::endpoint(tcp::v4(), port_));
  acceptor_->async_accept([this](std::error_code ec, asio::ip::tcp::socket s) {
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
  asio::async_connect(
      socket_, eps, [this](std::error_code ec, const tcp::endpoint &) {
        if (ec) {
          std::cerr << "[LINK] connect error: " << ec.message() << "\n";
          return;
        }
        connected_ = true;
        start_read_();
      });
}

void TcpLink::start_read_() {
  auto buf = std::make_shared<std::array<uint8_t, 1>>();
  socket_.async_read_some(
      asio::buffer(*buf), [this, buf](std::error_code ec, std::size_t n) {
        if (!ec && n == 1) {
          {
            std::lock_guard<std::mutex> lock(rx_mtx_);
            rx_q_.push((*buf)[0]);
          }
          start_read_();
        } else {
          if (ec)
            std::cerr << "[LINK] read error: " << ec.message() << "\n";
          connected_ = false;
          close_();
        }
      });
}

void TcpLink::close_() {
  std::error_code ec_sock, ec_acc;

  if (socket_.is_open()) {
    auto rc = socket_.close(ec_sock); // 某些Asio版本此处有返回值
    (void)rc;                         // 避免 [[nodiscard]] 警告
    if (ec_sock) {
      std::cerr << "[LINK] socket close error: " << ec_sock.message() << "\n";
    }
  }
  if (acceptor_ && acceptor_->is_open()) {
    auto rc = acceptor_->close(ec_acc); // 同上
    (void)rc;
    if (ec_acc) {
      std::cerr << "[LINK] acceptor close error: " << ec_acc.message() << "\n";
    }
  }
  connected_ = false;
}