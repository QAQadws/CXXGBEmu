#include "timer.h"
#include "defs.h"
#include <iostream>
#include "emu.h"
void TIMER::init()
{
    div_ = 0xAC00;
    tima_ = 0;
    tma_ = 0;
    tac_ = 0xF8;
}

void TIMER::tick(EMU* emu)
{
    u16 old_div = div_;
    ++div_; // DIV 每个机器周期增加1
    bool tima_update = false;
    switch (clock_select()) {
    case 0:
      tima_update = (old_div & (1 << 9)) && (!(div_ & (1 << 9)));
      break;
    case 1:
      tima_update = (old_div & (1 << 3)) && (!(div_ & (1 << 3)));
      break;
    case 2:
      tima_update = (old_div & (1 << 5)) && (!(div_ & (1 << 5)));
      break;
    case 3:
      tima_update = (old_div & (1 << 7)) && (!(div_ & (1 << 7)));
      break;
    }
    if (tima_update) {
      if (tima_ == 0xFF) {
        emu->int_flags |= INT_TIMER;
        tima_ = tma_;
      } else {
        ++tima_;
      }
    }
}

u8 TIMER::bus_read(u16 address)
{
   if(address == 0xFF04) return read_div();
   else if(address == 0xFF05) return tima_;
   else if(address == 0xFF06) return tma_;
   else if(address == 0xFF07) return tac_;
   else {
         std::cerr << "Invalid TIMER read at address: " << std::hex << address << std::endl;
          exit(EXIT_FAILURE);
         return 0xFF;
    }
}

void TIMER::bus_write(u16 address, u8 value)
{
    if(address == 0xFF04) {
        div_ = 0; // Writing any value to DIV resets it to 0
    }
    else if(address == 0xFF05) {
        tima_ = value;
    }
    else if(address == 0xFF06) {
        tma_ = value;
    }
    else if(address == 0xFF07) {
        tac_ = 0xF8 | (value & 0x07); // Only lower 3 bits are used
    }
    else {
        std::cerr << "Invalid TIMER write at address: " << std::hex << address << std::endl;
        exit(EXIT_FAILURE);
    }
    return;
}
