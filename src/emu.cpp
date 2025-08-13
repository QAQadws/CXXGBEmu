#include"emu.h"
#include <cstdlib>
#include <memory>
#include <iostream>


EMU::EMU(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: .\\GBemu.exe <rom_file> \n";
        exit(EXIT_FAILURE);
    }
    else if(argc == 2) {
        cart_ = std::make_shared<CART>(argv[1]);
        cpu_ = CPU();
        scale = 1.0; // 默认缩放比例为1.0
    }
    else{
      cart_ = std::make_shared<CART>(argv[1]);
        cpu_ = CPU();
        scale = atof(argv[2]);
    }
    clock_cycles_ = 0;
    int_flags = 0; // 初始化中断标志寄存器
    int_enable_flags = 0; // 初始化中断使能寄存器
    paused_ = false; // 初始化暂停状态为 false
    timer_.init();
    serial_.init();
    ppu_.init();
}

void EMU::update(f64 dt)
{
 u64 frame_cycles = static_cast<u64>(dt * 4194304.0 * scale); // 计算当前帧的时钟周期数
 u64 end_cycles = clock_cycles_ + frame_cycles;
 while(clock_cycles_ < end_cycles) {
    if(paused_) {break;}
    cpu_.step(this);
 }
}

void EMU::tick(u32 mcycles) {
  u32 tick_cycles = mcycles * 4; // 每个机器周期包含4个时钟周期
  for (u32 i = 0; i < tick_cycles; ++i) {
    ++clock_cycles_;
    timer_.tick(this);
    if((clock_cycles_ % 512) ==0) {
      serial_.tick(this);
    }
    ppu_.tick(this);

  }
}

u8 EMU::bus_read(u16 addressess)
{
  if (addressess <= 0x7FFF) {
    // Cartridge ROM.
    return cart_->rom_data_[addressess];
  }
  if (addressess <= 0x9FFF) {
    // VRAM.
    return vram[addressess - 0x8000];
  }
  if (addressess <= 0xBFFF) {
    // Cartridge RAM.
    //TODO
    std::cerr << "Reading from cartridge RAM is not implemented yet." << std::endl;
    exit(EXIT_FAILURE);
    return 0xFF;
  }
  if (addressess <= 0xDFFF) {
    // Working RAM.
    return wram[addressess - 0xC000];
  }
  if(addressess >= 0xFE00 && addressess <= 0xFE9F) {
    // OAM.
    return oam[addressess - 0xFE00];
  }
  if(addressess >= 0xFF01 && addressess <= 0xFF02) {
    // Serial transfer registers.
    return serial_.bus_read(addressess);
  }
  if(addressess>=0xFF04 && addressess <= 0xFF07) {
    // Timer registers.
    return timer_.bus_read(addressess);
  }
  if(addressess == 0xFF0F) {
    // Interrupt flags register.
    return int_flags | 0xE0; // 0xE0 is the default value for unused bits
  }
  if(addressess >= 0xFF40 && addressess <= 0xFF4B) {
    // PPU registers.
    return ppu_.bus_read(addressess);
  }
  if (addressess >= 0xFF80 && addressess <= 0xFFFE) {
    // High RAM.
    return hram[addressess - 0xFF80];
  }
  if(addressess == 0xFFFF) {
    // Interrupt enable register.
    return int_enable_flags | 0xE0; // 0xE0 is the default value for unused bits
  }
  std::cerr << "Invalid memory read at addressess: " << std::hex << addressess << std::endl;
  //exit(EXIT_FAILURE);
  return 0xFF;
}

void EMU::bus_write(u16 address, u8 value)
{
  if (address <= 0x7FFF) {
    // Cartridge ROM.
   cart_->rom_data_[address] = value;
    return;
  }
  if (address <= 0x9FFF) {
    // VRAM.
    vram[address - 0x8000] = value;
    return;
  }
  if (address <= 0xBFFF) {
    // Cartridge RAM.
    //TODO
    std::cerr << "Writing to cartridge RAM is not implemented yet." << std::endl;
    return;
  }
  if (address <= 0xDFFF) {
    // Working RAM.
    wram[address - 0xC000] = value;
    return;
  }
  if(address >= 0xFE00 && address <= 0xFE9F) {
    // OAM.
    oam[address - 0xFE00] = value;
    return;
  }
  if(address >= 0xFF01 && address <= 0xFF02) {
    // Serial transfer registers.
    serial_.bus_write(address, value);
    return;
  }
  if(address >= 0xFF04 && address <= 0xFF07) {
    // Timer registers.
    timer_.bus_write(address, value);
    return;
  }
  if(address == 0xFF0F) {
    // Interrupt flags register.
    int_flags = value & 0x1F; // 保留高位
    return;
  }
  if(address >= 0xFF40 && address <= 0xFF4B) {
    // PPU registers.
    ppu_.bus_write(address, value);
    return;
  }
  if (address >= 0xFF80 && address <= 0xFFFE) {
    // High RAM.
    hram[address - 0xFF80] = value;
    return;
  }
  if(address == 0xFFFF) {
    // Interrupt enable register.
    int_enable_flags = value & 0x1F; // 保留高位
    return;
  }
  std::cerr << "Invalid memory write at address: " << std::hex << address << std::endl;
  //exit(EXIT_FAILURE);
  return;
}
// 0x0000 - 0x3FFF : ROM Bank 0
// 0x4000 - 0x7FFF : ROM Bank 1 - Switchable
// 0x8000 - 0x97FF : CHR RAM
// 0x9800 - 0x9BFF : BG Map 1
// 0x9C00 - 0x9FFF : BG Map 2
// 0xA000 - 0xBFFF : Cartridge RAM
// 0xC000 - 0xCFFF : RAM Bank 0
// 0xD000 - 0xDFFF : RAM Bank 1-7 - switchable - Color only
// 0xE000 - 0xFDFF : Reserved - Echo RAM
// 0xFE00 - 0xFE9F : Object Attribute Memory
// 0xFEA0 - 0xFEFF : Reserved - Unusable
// 0xFF00 - 0xFF7F : I/O Registers
// 0xFF80 - 0xFFFE : Zero Page