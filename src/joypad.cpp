#include"joypad.h"
#include"emu.h"

void JOYPAD::init() {
    p1 = 0xFF;
    a = false;
    b = false;
    select = false;
    start = false;
    right = false;
    left = false;
    up = false;
    down = false;
}

void JOYPAD::update(EMU *emu)
{
    u8 v = get_key_state();
    if(((p1 & 0x01) && !(v & 0x01))||
       ((p1 & 0x02) && !(v & 0x02))||
       ((p1 & 0x04) && !(v & 0x04))||
       ((p1 & 0x08) && !(v & 0x08)))
    {
      emu->int_flags |= INT_JOYPAD;
    }
    p1 = v;
}

u8 JOYPAD::bus_read()
{
    return p1;
}

void JOYPAD::bus_write(u8 data)
{
  p1 = (data & 0x30) | (p1 & 0xCF);
  p1 = get_key_state();
}

u8 JOYPAD::get_key_state()const
{
    u8 v = 0xFF;
    if(!(p1>> 4 & 0x01)){
        if(right) v &= ~0x01;
        if(left) v &= ~0x02;
        if(up) v &= ~0x04;
        if(down) v &= ~0x08;
        v&= ~(0x01<<4);
    }
    if(!(p1 >> 5 & 0x01)){
        if(a) v &= ~0x01;
        if(b) v &= ~0x02;
        if(select) v &= ~0x04;
        if(start) v &= ~0x08;
        v &= ~(0x01 << 5);
    }
    return v;
}
