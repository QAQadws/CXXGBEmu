#pragma once
#include <cstdint>

struct ILink {
  virtual ~ILink() = default;
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual bool connected() const = 0;

  // 非阻塞尝试弹出一个收到的字节；有则写入out并返回true
  virtual bool try_recv(uint8_t &out) = 0;

  // 异步发送一个字节（若未连接则忽略）
  virtual void send(uint8_t b) = 0;
};