#include"cpu.h"
#include "defs.h"
#include"emu.h"
#include "instructions.h"
#include<iostream>
#ifdef DEBUG
#include<format>
#endif

#ifdef DEBUG
static u32 count = 0;
#endif
CPU::CPU()
{
  af(0x01B0);//0x01B0 = 0000 0001 1011 0000
  bc(0x0013);
  de(0x00D8);
  hl(0x014D);
  sp = 0xFFFE;
  pc = 0x0100;
  halted_ = false;
  interrupt_master_enabled_ = false; // 初始化中断使能状态为 false
  interrupt_master_enabling_countdown_ = 0; // 初始化中断使能倒计时为 0
}

void CPU::step(EMU *emu) {
  if (halted_) {
    emu->tick(1);
    if ((emu->int_flags &emu->int_enable_flags)) {
      halted_ = false;
    }
  } else {
    if (interrupt_master_enabled_ && (emu->int_flags & emu->int_enable_flags)) {
      service_interrupt(emu);
    } else {
      u8 opcode = emu->bus_read(pc);
      pc++;
      instruction_func_t instruction = instructions_map[opcode];
      if (!instruction) {
        std::cerr <<"\t"<< "Unknown opcode: " << std::hex << (int)opcode << std::endl;
        emu->paused_ = true; // 停止执行
      } else {
#ifdef DEBUG
        std::string flags =
            std::format("{}{}{}{}", (emu->cpu_.f & (1 << 7)) ? 'Z' : '-',
                        (emu->cpu_.f & (1 << 6)) ? 'N' : '-',
                        (emu->cpu_.f & (1 << 5)) ? 'H' : '-',
                        (emu->cpu_.f & (1 << 4)) ? 'C' : '-');
        printf("%08llX - %04X: %-16s (%02X %02X %02X) A: %02X  F: %s  BC: %04X  DE: %04X  HL: %04X\n",
          emu->clock_cycles_,
          pc - 1,
          instruction_names[opcode].c_str(),
          emu->bus_read(pc - 1),
          emu->bus_read(pc),
          emu->bus_read(pc + 1),
          emu->cpu_.a,
          flags.c_str(),
          emu->cpu_.bc(),
          emu->cpu_.de(),
          emu->cpu_.hl());

          dbg_.dbg_update(emu);
          dbg_.dbg_print();
          count++;

#endif
        instruction(emu);
      }
    }
  }
  if (interrupt_master_enabling_countdown_) {
    --interrupt_master_enabling_countdown_;
    if (!interrupt_master_enabling_countdown_) {
      interrupt_master_enabled_ = true;
    }
  }
}

inline void push_16(EMU *emu, u16 value) {
  emu->cpu_.sp -= 2;
  emu->bus_write(emu->cpu_.sp + 1, static_cast<u8>((value >> 8) & 0xFF));
  emu->bus_write(emu->cpu_.sp, static_cast<u8>(value & 0xFF));
}
void CPU::service_interrupt(EMU *emu)
{
      u8 int_flags = emu->int_flags & emu->int_enable_flags;
    u8 service_int = 0;

    if(int_flags & INT_VBLANK) service_int = INT_VBLANK;
    else if(int_flags & INT_LCD_STAT) service_int = INT_LCD_STAT;
    else if(int_flags & INT_TIMER) service_int = INT_TIMER;
    else if(int_flags & INT_SERIAL) service_int = INT_SERIAL;
    else if(int_flags & INT_JOYPAD) service_int = INT_JOYPAD;
    emu->int_flags &= ~service_int;
    emu->cpu_.disable_interrupt_master();
    emu->tick(2);
    push_16(emu, emu->cpu_.pc);
    emu->tick(2);
    switch(service_int)
    {
        case INT_VBLANK: emu->cpu_.pc = 0x40; break;
        case INT_LCD_STAT: emu->cpu_.pc = 0x48; break;
        case INT_TIMER: emu->cpu_.pc = 0x50; break;
        case INT_SERIAL: emu->cpu_.pc = 0x58; break;
        case INT_JOYPAD: emu->cpu_.pc = 0x60; break;
    }
    emu->tick(1);
}

void CPU::enable_interrupt_master()
{
    interrupt_master_enabling_countdown_ = 2; // 设置倒计时为2个周期
}

void CPU::disable_interrupt_master()
{
    interrupt_master_enabled_ = false; // 禁用中断
    interrupt_master_enabling_countdown_ = 0; // 重置倒计时
}