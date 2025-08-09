#ifndef __TIMER_H__
#define __TIMER_H__
#include "defs.h"

class EMU; // 前向声明
class TIMER {
public:
  TIMER() = default;
  void init();
  void tick(EMU* emu);
  u8 bus_read(u16 address);
  void bus_write(u16 address, u8 value);

  u8 read_div()const { return div_ >> 8; } 
  u8 clock_select() const {return (tac_ & 0x03);}
  bool tima_enabled() const {return (tac_ & 0x04) != 0;}

private:
  u16 div_ = 0xAC00;  // Divider Register 0xFF04
  u8 tima_ = 0; // Timer Counter Register 0xFF05
  u8 tma_ = 0;  // Timer Modulo Register 0xFF06
  u8 tac_ = 0xF8;  // Timer Control Register 0xFF07
};

#endif // __TIMER_H__