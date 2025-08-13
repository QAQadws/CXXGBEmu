#ifndef __EMU_H__
#define __EMU_H__
#include"cart.h"
#include"cpu.h"
#include"defs.h"
#include"timer.h"
#include"serial.h"
#include"ppu.h"

class EMU {
 public:
   EMU() = default;
   EMU(int argc, char *argv[]);
   void update(f64 dt);
   void tick(u32 mcycles);

   u8 bus_read(u16 address);
   void bus_write(u16 address, u8 value);
   std::shared_ptr<CART> get_cart() { return cart_; }

   bool paused_ = false; // 暂停状态
   CPU cpu_;
   u8 vram[0x2000]{}; // 8kb
   u8 wram[0x2000]{}; // 8kb
   u8 hram[128]{};
   u8 oam[160]{};
   u8 int_flags = 0; // 中断标志寄存器
   //0xFFFF
   u8 int_enable_flags = 0; // 中断使能寄存器
   TIMER timer_;
   SERIAL serial_;
   std::shared_ptr<CART> cart_;
   u64 clock_cycles_;
   double scale = 1.0f; // 默认缩放比例为1.0
   PPU ppu_;

 private:

};


#endif // __EMU_H__