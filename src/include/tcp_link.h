#pragma once
#define ASIO_STANDALONE
#include "link.h"
#include <asio.hpp>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>


class TcpLink final : public ILink {
public:
  // server: server=true, host无视，仅需port
  // client: server=false, host="192.168.x.x", port同服务端
  TcpLink(bool server, const std::string &host, uint16_t port)
      : server_(server), host_(host), port_(port), socket_(ioc_) {}

  void start() override;
  void stop() override;
  bool connected() const override { return connected_; }
  bool try_recv(uint8_t &out) override;
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