#ifndef __INSTRUCTIONS_H__
#define __INSTRUCTIONS_H__
#include "emu.h"
typedef void (*instruction_func_t)(EMU *emu);
// 定义一个指令映射表，包含256个指令，每个指令对应一个函数指针
extern instruction_func_t instructions_map[256];

#endif // __INSTRUCTIONS_H__