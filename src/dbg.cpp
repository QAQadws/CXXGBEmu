#include"dbg.h"
#include "emu.h"
void DBG::dbg_update(EMU* emu)
{   
    
}

void DBG::dbg_print()
{
    if(dbg_msg[0]) {
        printf("DBG: %s\n", dbg_msg);
    }
}
