#ifndef __CPU_H__
#define __CPU_H__
#include"defs.h"
class EMU;

class CPU {
public:
u16 pc;  // Program counter
u16 sp;  // Stack pointer
u8 a, f; // Accumulator and flags
u8 b, c; // General purpose registers
u8 d, e;
u8 h, l;
bool halted_; // Halted state
bool interrupt_master_enabled_ = false; // 中断使能状态
u8 interrupt_master_enabling_countdown_ = 0; // 中断使能倒计时


  CPU();
  void step(EMU *emu);

  void service_interrupt(EMU *emu);
  void enable_interrupt_master();
  void disable_interrupt_master();
  u16 af() const { return (static_cast<u16>(a) << 8) | f; }
  u16 bc() const { return (static_cast<u16>(b) << 8) | c; }
  u16 de() const { return (static_cast<u16>(d) << 8) | e; }
  u16 hl() const { return (static_cast<u16>(h) << 8) | l; }
  void af(u16 v) { a = (u8)(v >> 8); f = (u8)(v & 0xF0); }
  void bc(u16 v) { b = (u8)(v >> 8); c = (u8)(v & 0xFF); }
  void de(u16 v) { d = (u8)(v >> 8); e = (u8)(v & 0xFF); }
  void hl(u16 v) { h = (u8)(v >> 8); l = (u8)(v & 0xFF); }
  bool fz() const { return (f & 0x80) != 0; }
  bool fn() const { return (f & 0x40) != 0; }
  bool fh() const { return (f & 0x20) != 0; }
  bool fc() const { return (f & 0x10) != 0; }
  void set_fz() { f |= 0x80; }
  void reset_fz() { f &= 0x7F; }
  void set_fn() { f |= 0x40; }
  void reset_fn() { f &= 0xBF; }
  void set_fh() { f |= 0x20; }
  void reset_fh() { f &= 0xDF; }
  void set_fc() { f |= 0x10; }
  void reset_fc() { f &= 0xEF; }
};

#endif // __CPU_H__