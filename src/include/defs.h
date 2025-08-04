#ifndef __DEFS_H__
#define __DEFS_H__

#include<cstdint>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef float f32;
typedef double f64;

#define DEBUG

constexpr u8 INT_VBLANK = 0x01;   // 二进制: 00000001
constexpr u8 INT_LCD_STAT = 0x02; // 二进制: 00000010
constexpr u8 INT_TIMER = 0x04;    // 二进制: 00000100
constexpr u8 INT_SERIAL = 0x08;   // 二进制: 00001000
constexpr u8 INT_JOYPAD = 0x10;   // 二进制: 00010000
#endif // __DEFS_H__