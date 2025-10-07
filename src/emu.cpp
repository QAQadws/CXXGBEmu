#include "emu.h"
#include "cart.h"
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <tcp_link.h>

EMU::EMU(int argc, char *argv[])
{
    rtc_.init();
    if (argc < 2) {
        std::cerr << "Usage: .\\GBemu.exe <rom_file> \n";
        exit(EXIT_FAILURE);
    }
    else if(argc == 2) {
        cart_ = std::make_shared<CART>(argv[1]);
        cpu_ = CPU();
        scale = 1.0; // 默认缩放比例为1.0
    }
    else if(argc == 3) {
        cart_ = std::make_shared<CART>(argv[1]);
        cpu_ = CPU();
        scale = atof(argv[2]);
        // 建立服务端（等待连接）
        link_server = std::make_unique<TcpLink>(true, "", 8765);
        link_server->start();
        serial_.attach_link(link_server.get());
      }
    else if(argc == 4) {
        cart_ = std::make_shared<CART>(argv[1]);
        cpu_ = CPU();
        scale = atof(argv[2]);
        std::string host_ = argv[3];
        // 建立客户端并连接到指定主机
        link_client = std::make_unique<TcpLink>(false, host_, 8765);
        link_client->start();
        serial_.attach_link(link_client.get());
    }
      rom_name_ = argv[1];
      // 去掉 .gb 扩展名
      size_t dot_pos = rom_name_.find_last_of('.');
      if (dot_pos != std::string::npos) {
        rom_name_ = rom_name_.substr(0, dot_pos);
      }
    switch(cart_->header_->ram_size){
      case 2 :cram_size_ = 0x02000; break; // 8KB
      case 3 :cram_size_ = 0x08000; break; // 32KB
      case 4 :cram_size_ = 0x20000; break; // 128KB
      case 5 :cram_size_ = 0x10000; break; // 64KB
      default: cram_size_ = 0; break;
    }
    if (cart_->is_cart_mbc2(cart_->header_->type)) {
      cram_size_ = 512;
    }
    if(cram_size_){
      cram_ = std::shared_ptr<u8[]>(new u8[cram_size_]());
      if(cart_->is_cart_battery(cart_->header_->type)) {
        load_cartridge_ram_data();
        std::cerr << "Loaded cartridge RAM data from file." << std::endl;
      }
    }
    num_rom_banks_ = ((((u32)32) << cart_->header_->rom_size) / 16);
    clock_cycles_ = 0;
    int_flags = 0; // 初始化中断标志寄存器
    int_enable_flags = 0; // 初始化中断使能寄存器
    paused_ = false; // 初始化暂停状态为 false
    timer_.init();
    serial_.init();
    ppu_.init();
    joypad_.init();
    apu.init();
}

void EMU::update(f64 dt)
{
 joypad_.update(this);
  if(cart_->is_cart_timer(cart_->header_->type)) {
    rtc_.update(dt);
  }
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
    apu.tick(this);

  }
}

void EMU::clean()
{
  if (cram_) {
    save_cartridge_ram_data();
  }
}

u8 EMU::bus_read(u16 address)
{
  if (address <= 0x7FFF) {
    // Cartridge ROM.
    return cart_->cartridge_read(this, address);
  }
  if (address <= 0x9FFF) {
    // VRAM.
    return vram[address - 0x8000];
  }
  if (address <= 0xBFFF) {
    // Cartridge RAM.
    if(cram_){
      return cart_->cartridge_read(this, address);
    }
    std::cerr << "Reading from cartridge RAM is not implemented yet." << std::endl;
    return 0xFF;
  }
  if (address <= 0xDFFF) {
    // Working RAM.
    return wram[address - 0xC000];
  }
  if(address >= 0xFE00 && address <= 0xFE9F) {
    // OAM.
    return oam[address - 0xFE00];
  }
  if(address == 0xFF00) {
    // Joypad register.
    return joypad_.bus_read();
  }
  if(address >= 0xFF01 && address <= 0xFF02) {
    // Serial transfer registers.
    return serial_.bus_read(address);
  }
  if(address>=0xFF04 && address <= 0xFF07) {
    // Timer registers.
    return timer_.bus_read(address);
  }
  if(address == 0xFF0F) {
    // Interrupt flags register.
    return int_flags | 0xE0; // 0xE0 is the default value for unused bits
  }
  if (address >= 0xFF10 && address <= 0xFF3F) {
    return apu.bus_read(address);
  }
  if(address >= 0xFF40 && address <= 0xFF4B) {
    // PPU registers.
    return ppu_.bus_read(address);
  }
  if (address >= 0xFF80 && address <= 0xFFFE) {
    // High RAM.
    return hram[address - 0xFF80];
  }
  if(address == 0xFFFF) {
    // Interrupt enable register.
    return int_enable_flags | 0xE0; // 0xE0 is the default value for unused bits
  }
  //std::cerr << "Invalid memory read at address: " << std::hex << address << std::endl;
  return 0xFF;
}

void EMU::bus_write(u16 address, u8 value)
{
  if (address <= 0x7FFF) {
    // Cartridge ROM.
    cart_->cartridge_write(this, address, value);
      return;
  }
  if (address <= 0x9FFF) {
    // VRAM.
    vram[address - 0x8000] = value;
    return;
  }
  if (address <= 0xBFFF) {
    // Cartridge RAM.
    if(cram_) {
      cart_->cartridge_write(this, address, value);
      return;
    }
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
  if(address == 0xFF00) {
    // Joypad register.
    joypad_.bus_write(value);
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
  if (address >= 0xFF10 && address <= 0xFF3F) {
    apu.bus_write(address, value);
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
  // std::cerr << "Invalid memory write at address: " << std::hex << address << std::endl;
  return;
}

void EMU::load_cartridge_ram_data() {
  try {
    if (!cram_ || cram_size_ == 0) {
      return;
    }

    std::string load_path = "roms/" + rom_name_ + ".sav";
    std::ifstream file(load_path, std::ios::binary);

    if (!file.is_open()) {
      std::cout << "No save file found: " << load_path << std::endl;
      return;
    }

    // ✅ 检查文件大小
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    // 计算期望的文件大小

    if (file_size < cram_size_) {
      std::cerr << "Save file too small: " << file_size << " < " << cram_size_
                << std::endl;
      file.close();
      return;
    }

    // ✅ 读取卡带RAM数据
    file.read(reinterpret_cast<char *>(cram_.get()), cram_size_);

    // ✅ 检查是否是带RTC的卡带
    if (cart_->is_cart_timer(cart_->header_->type)) {
      // 检查文件是否包含RTC数据
      if (file_size >= cram_size_ + sizeof(RTC) + sizeof(s64)) {
        // 读取RTC状态
        file.read(reinterpret_cast<char *>(&rtc_), sizeof(RTC));

        // 读取保存时的时间戳
        s64 save_timestamp;
        file.read(reinterpret_cast<char *>(&save_timestamp), sizeof(s64));

        if (file.good()) {
          // 如果RTC没有暂停，计算经过的时间
          if (!rtc_.halted()) {
            s64 current_timestamp = get_utc_timestamp();
            s64 delta_time = current_timestamp - save_timestamp;

            if (delta_time < 0)
              delta_time = 0; // 防止时间倒退

            // 更新RTC时间
            rtc_.time += delta_time;
            rtc_.update_time_registers();

            std::cout << "RTC restored with delta time: " << delta_time
                      << " seconds" << std::endl;
          } else {
            std::cout << "RTC is halted, time not advanced" << std::endl;
          }
        } else {
          std::cerr << "Failed to read RTC data" << std::endl;
        }
      } else {
        std::cout << "No RTC data found in save file" << std::endl;
        // 初始化RTC到当前时间
        rtc_.init();
      }
    }

    if (file.good()) {
      std::cout << "Cartridge RAM data loaded: " << load_path << std::endl;
    } else {
      std::cerr << "Failed to read cartridge RAM data from: " << load_path
                << std::endl;
    }

    file.close();

  } catch (const std::exception &e) {
    std::cerr << "Exception while loading cartridge RAM: " << e.what()
              << std::endl;
  }
}

void EMU::save_cartridge_ram_data() {
  try {
    if (!cram_ || cram_size_ == 0) {
      return;
    }

    std::string save_path = "roms/" + rom_name_ + ".sav";
    std::ofstream file(save_path, std::ios::binary | std::ios::trunc);

    if (!file.is_open()) {
      std::cerr << "Failed to open save file: " << save_path << std::endl;
      return;
    }

    // ✅ 写入卡带RAM数据
    file.write(reinterpret_cast<const char *>(cram_.get()), cram_size_);

    // ✅ 检查是否是带RTC的卡带
    if (cart_->is_cart_timer(cart_->header_->type)) {
      // 保存RTC状态
      file.write(reinterpret_cast<const char *>(&rtc_), sizeof(RTC));

      // 保存当前时间戳
      s64 timestamp = get_utc_timestamp();
      file.write(reinterpret_cast<const char *>(&timestamp), sizeof(s64));

      std::cout << "RTC data saved with timestamp: " << timestamp << std::endl;
    }

    if (file.good()) {
      std::cout << "Cartridge RAM data saved: " << save_path << std::endl;
    } else {
      std::cerr << "Failed to write cartridge RAM data to: " << save_path
                << std::endl;
    }

    file.close();

  } catch (const std::exception &e) {
    std::cerr << "Exception while saving cartridge RAM: " << e.what()
              << std::endl;
    std::cerr << "Failed to save cartridge RAM data. Game progress may lost!"
              << std::endl;
  }
}

s64 EMU::get_utc_timestamp()
{
  
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
  
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