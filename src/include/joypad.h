#ifndef __JOYPAD_H__
#define __JOYPAD_H__
#include"defs.h"
class EMU;
class JOYPAD{
public:
    void init();
    void update(EMU *emu);
    u8 bus_read();
    void bus_write(u8 data);
    u8 get_key_state()const;

    u8 p1;
    bool a;
    bool b;
    bool select;
    bool start;
    bool right;
    bool left;
    bool up;
    bool down;
};

#endif // __JOYPAD_H__