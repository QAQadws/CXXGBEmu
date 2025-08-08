#ifndef __DBG_H__
#define __DBG_H__
#include"defs.h"
class EMU;
class DBG {
public:
    DBG() = default;
    void dbg_update(EMU* emu);
    void dbg_print();
    char dbg_msg[1024]{};
    u8 dbg_size{};
};

#endif // __DBG_H__