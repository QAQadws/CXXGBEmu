#ifndef __EMU_H__
#define __EMU_H__
#include"cart.h"
#include"cpu.h"
#include"defs.h"
#include"timer.h"
#include"serial.h"
#include"ppu.h"
#include"joypad.h"
#include <string>
#include"rtc.h"


class EMU {
 public:
   EMU() = default;
   EMU(int argc, char *argv[]);
   void update(f64 dt);
   void tick(u32 mcycles);
   void clean();

   u8 bus_read(u16 address);
   void bus_write(u16 address, u8 value);
   void load_cartridge_ram_data();
   void save_cartridge_ram_data();
   s64 get_utc_timestamp();

   bool paused_ = false; // 暂停状态
   CPU cpu_;
   u8 vram[0x2000]{}; // 8kb = 0010 0000 0000 0000
   u8 wram[0x2000]{}; // 8kb = 0010 0000 0000 0000
   u8 hram[128]{};
   u8 oam[160]{};
   u8 int_flags = 0; // 中断标志寄存器
   //0xFFFF
   u8 int_enable_flags = 0; // 中断使能寄存器
   TIMER timer_;
   SERIAL serial_;
   std::shared_ptr<CART> cart_;
   std::string rom_name_;
   std::shared_ptr<u8[]> cram_;
   u32 cram_size_{};

   u8 num_rom_banks_{};
   bool cram_enabled_ = false;
   u8 rom_bank_number = 1;
   u8 ram_bank_number = 0;
   u8 banking_mode = 0;

   u64 clock_cycles_;
   double scale = 1.0f; // 默认缩放比例为1.0
   PPU ppu_;
   JOYPAD joypad_;
   RTC rtc_;



};


#endif // __EMU_H__