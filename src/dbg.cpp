#include"dbg.h"
#include "emu.h"
void DBG::dbg_update(EMU* emu)
{   

    if(!emu->serial_.output_buffer_.empty()) {
        u8 c = emu->serial_.output_buffer_.front();
        dbg_msg[dbg_size++] = c;
        emu->serial_.output_buffer_.pop_front();
    }
}

void DBG::dbg_print()
{
    if(dbg_msg[0]) {
        printf("DBG: %s\n", dbg_msg);

    }
}
