#ifndef __SERIAL_H__
#define __SERIAL_H__
#include"defs.h"
#include<deque>
class EMU; // 前向声明
class SERIAL {
private:
  u8 sb_;             // 发送缓冲区
  u8 sc_;             // 控制寄存器
  bool transferring_; // 是否正在传输数据
  u8 out_byte_;
  s8 transfer_bit_;
  
  public:
  std::deque<u8> output_buffer_;
  void init();
  void tick(EMU* emu);

    void begin_transfer();
    void process_transfer(EMU* emu);
    void end_transfer(EMU* emu);

    bool is_master() const { return (sc_ & 0x01); }
    bool transfer_enable() const { return (sc_ & 0x80); }
    void bus_write(u16 address, u8 data);
    u8 bus_read(u16 address);
};

#endif // __SERIAL_H__