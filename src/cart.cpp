#include "cart.h"
#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <cstring>
#include "defs.h"
#include"emu.h"

std::string CART::cart_type_name() const
{
  std::string type_names[] = {
      "ROM ONLY",
      "MBC1",
      "MBC1+RAM",
      "MBC1+RAM+BATTERY",
      "0x04 ???",
      "MBC2",
      "MBC2+BATTERY",
      "0x07 ???",
      "ROM+RAM 1",
      "ROM+RAM+BATTERY 1",
      "0x0A ???",
      "MMM01",
      "MMM01+RAM",
      "MMM01+RAM+BATTERY",
      "0x0E ???",
      "MBC3+TIMER+BATTERY",
      "MBC3+TIMER+RAM+BATTERY 2",
      "MBC3",
      "MBC3+RAM 2",
      "MBC3+RAM+BATTERY 2",
      "0x14 ???",
      "0x15 ???",
      "0x16 ???",
      "0x17 ???",
      "0x18 ???",
      "MBC5",
      "MBC5+RAM",
      "MBC5+RAM+BATTERY",
      "MBC5+RUMBLE",
      "MBC5+RUMBLE+RAM",
      "MBC5+RUMBLE+RAM+BATTERY",
      "0x1F ???",
      "MBC6",
      "0x21 ???",
      "MBC7+SENSOR+RUMBLE+RAM+BATTERY",
  };
  if(header_->type<=0x22){
  return type_names[header_->type];
  }
  return "UNKNOWN";
}

std::string CART::cart_lic_code() const
{
    std::string lic_codes[0xA5];
        lic_codes[0x00] = "None";
        lic_codes[0x01] = "Nintendo R&D1";
        lic_codes[0x08] = "Capcom";
        lic_codes[0x13] = "Electronic Arts";
        lic_codes[0x18] = "Hudson Soft";
        lic_codes[0x19] = "b-ai";
        lic_codes[0x20] = "kss";
        lic_codes[0x22] = "pow";
        lic_codes[0x24] = "PCM Complete";
        lic_codes[0x25] = "san-x";
        lic_codes[0x28] = "Kemco Japan";
        lic_codes[0x29] = "seta";
        lic_codes[0x30] = "Viacom";
        lic_codes[0x31] = "Nintendo";
        lic_codes[0x32] = "Bandai";
        lic_codes[0x33] = "Ocean/Acclaim";
        lic_codes[0x34] = "Konami";
        lic_codes[0x35] = "Hector";
        lic_codes[0x37] = "Taito";
        lic_codes[0x38] = "Hudson";
        lic_codes[0x39] = "Banpresto";
        lic_codes[0x41] = "Ubi Soft";
        lic_codes[0x42] = "Atlus";
        lic_codes[0x44] = "Malibu";
        lic_codes[0x46] = "angel";
        lic_codes[0x47] = "Bullet-Proof";
        lic_codes[0x49] = "irem";
        lic_codes[0x50] = "Absolute";
        lic_codes[0x51] = "Acclaim";
        lic_codes[0x52] = "Activision";
        lic_codes[0x53] = "American sammy";
        lic_codes[0x54] = "Konami";
        lic_codes[0x55] = "Hi tech entertainment";
        lic_codes[0x56] = "LJN";
        lic_codes[0x57] = "Matchbox";
        lic_codes[0x58] = "Mattel";
        lic_codes[0x59] = "Milton Bradley";
        lic_codes[0x60] = "Titus";
        lic_codes[0x61] = "Virgin";
        lic_codes[0x64] = "LucasArts";
        lic_codes[0x67] = "Ocean";
        lic_codes[0x69] = "Electronic Arts";
        lic_codes[0x70] = "Infogrames";
        lic_codes[0x71] = "Interplay";
        lic_codes[0x72] = "Broderbund";
        lic_codes[0x73] = "sculptured";
        lic_codes[0x75] = "sci";
        lic_codes[0x78] = "THQ";
        lic_codes[0x79] = "Accolade";
        lic_codes[0x80] = "misawa";
        lic_codes[0x83] = "lozc";
        lic_codes[0x86] = "Tokuma Shoten Intermedia";
        lic_codes[0x87] = "Tsukuda Original";
        lic_codes[0x91] = "Chunsoft";
        lic_codes[0x92] = "Video system";
        lic_codes[0x93] = "Ocean/Acclaim";
        lic_codes[0x95] = "Varie";
        lic_codes[0x96] = "Yonezawa/s'pal";
        lic_codes[0x97] = "Kaneko";
        lic_codes[0x99] = "Pack in soft";
        lic_codes[0xA4] = "Konami (Yu-Gi-Oh!)";

        if (header_->lic_code <= 0xA5) {
      return std::string(lic_codes[header_->lic_code]);
    }
    return "UNKNOWN";
}

bool CART::is_cart_battery(u8 cartridge_type)
{
    return cartridge_type == 3 ||  // MBC1+RAM+BATTERY
           cartridge_type == 6 ||  // MBC2+BATTERY
           cartridge_type == 9 ||  // ROM+RAM+BATTERY 1
           cartridge_type == 13 || // MMM01+RAM+BATTERY
           cartridge_type == 15 || // MBC3+TIMER+BATTERY
           cartridge_type == 16 || // MBC3+TIMER+RAM+BATTERY 2
           cartridge_type == 19 || // MBC3+RAM+BATTERY 2
           cartridge_type == 27 || // MBC5+RAM+BATTERY
           cartridge_type == 30 || // MBC5+RUMBLE+RAM+BATTERY
           cartridge_type == 34;   // MBC7+SENSOR+RUMBLE+RAM+BATTERY
}

bool CART::is_cart_mbc1(u8 cartridge_type)
{
  return cartridge_type == 1 ||  // MBC1
         cartridge_type == 2 ||  // MBC1+RAM
         cartridge_type == 3;     // MBC1+RAM+BATTERY
}

bool CART::is_cart_mbc2(u8 cartridge_type)
{
  return cartridge_type >= 5 && cartridge_type <= 6;
}

bool CART::is_cart_mbc3(u8 cartridge_type)
{
  return cartridge_type >= 15 && cartridge_type <= 19;
}

bool CART::is_cart_timer(u8 cartridge_type)
{
  return cartridge_type == 15 || cartridge_type == 16;
}

bool CART::is_cart_mbc5(u8 cartridge_type)
{
  return cartridge_type >= 25 && cartridge_type <= 30;
}



bool CART::load_cart(const char *filename) {
  std::string temp = "roms/" + std::string(filename);
  std::cout << "Loading cartridge: " << temp << std::endl;
  std::ifstream romFile(temp.c_str(), std::ios::binary | std::ios::ate);

  if (romFile.is_open()) {

    rom_size_ = romFile.tellg();
    romFile.seekg(0, std::ios::beg);

    // 分配 ROM 数据内存 (类似 malloc)
    rom_data_ = std::make_unique<u8[]>(rom_size_);

    // 读取整个 ROM 文件
    if (romFile.read(reinterpret_cast<char *>(rom_data_.get()), rom_size_)) {
      // 分配 header 内存并从 ROM 数据的 0x100 位置复制
      // 类似 ctx.header = (rom_header *)(ctx.rom_data + 0x100);
      header_ = std::make_unique<rom_header>();

      if (rom_size_ >= 0x100 + sizeof(rom_header)) {
        // 从 ROM 数据的 0x100 偏移位置复制 header
        memcpy(header_.get(), rom_data_.get() + 0x100, sizeof(rom_header));
        header_->title[15] = 0; // 确保标题字符串以 null 结尾

        // 输出标题并对齐
        printf("Cartridge Loaded:\n");
        printf("\t Title    : %s\n", header_->title);
        printf("\t Type     : %2.2X (%s)\n", header_->type,
               cart_type_name().c_str());
        printf("\t ROM Size : %d KB\n", 32 << header_->rom_size);
        printf("\t RAM Size : %2.2X\n", header_->ram_size);
        printf("\t LIC Code : %2.2X (%s)\n", header_->lic_code,
               cart_lic_code().c_str());
        printf("\t ROM Vers : %2.2X\n", header_->version);

        u8 checksum = 0;
        for (u16 address = 0x0134; address <= 0x014C; address++) {
          checksum = checksum - rom_data_[address] - 1;
        }
        if(checksum == rom_data_[0x014D]) {
          printf("\t Checksum : %2.2X (OK)\n", checksum);
        } else {
          printf("\t Checksum : %2.2X (FAIL)\n", checksum);
        }
        return true;
      } else {
        std::cerr << "ROM file too small for header at 0x100" << std::endl;
        exit(EXIT_FAILURE);
      }
    } else {
      std::cerr << "Failed to read ROM data: " << filename << std::endl;
      
    }
  } else {
    std::cerr << "Failed to open ROM file: " << filename << std::endl;
    exit(EXIT_FAILURE);
  }
  return false;
}

u8 CART::cartridge_read(EMU* emu, u16 address)
{
  u8 cartridge_type =  header_->type;
  if(is_cart_mbc1(cartridge_type)){
    return mbc1_read(emu, address);
  }
  else if(is_cart_mbc2(cartridge_type)){
    return mbc2_read(emu, address);
  }
  // 如果是 MBC3
  else if(is_cart_mbc3(cartridge_type)){
    return mbc3_read(emu, address);
  }
  // 如果是 MBC5
  else if(is_cart_mbc5(cartridge_type)){
    return mbc5_read(emu, address);
  }
  else{
    if(address < 0x7FFF){
      return rom_data_[address];
    }
    if (address >= 0xA000 && address <= 0xBFFF && emu->cram_size_) {
      return emu->cram_[address - 0xA000];
    }
  }
  std::cerr << "Invalid cartridge read at address: " << std::hex << address << std::endl;
  return 0xFF;
}

void CART::cartridge_write(EMU* emu, u16 address, u8 value)
{
  u8 cartridge_type = header_->type;
  if (is_cart_mbc1(cartridge_type)) {
    mbc1_write(emu, address, value);
    return;
  } else if (is_cart_mbc2(cartridge_type)) {
    mbc2_write(emu, address, value);
    return;
  } else if (is_cart_mbc3(cartridge_type)) {
    mbc3_write(emu, address, value);
    return;
  } else if (is_cart_mbc5(cartridge_type)) {
    mbc5_write(emu, address, value);
    return;
  } else {
    if (address >= 0xA000 && address <= 0xBFFF && emu->cram_size_) {
      emu->cram_[address - 0xA000] = value;
      return;
    }
  }
}

u8 CART::mbc1_read(EMU* emu, u16 address)
{
  if(address <= 0x3FFF){
    if(emu->banking_mode && emu->num_rom_banks_ >32){
      u64 bank_index = emu->ram_bank_number;
      u64 bank_offset = bank_index * 32 * 0x4000;//0x4000 = 16KB
      return rom_data_[bank_offset + address];
    } else {
      return rom_data_[address];
    }
  }
  if (address >= 0x4000 && address <= 0x7FFF) {
    if (emu->banking_mode && emu->num_rom_banks_ > 32) {
      u64 bank_index = emu->rom_bank_number + (emu->ram_bank_number << 5);
      u64 bank_offset = bank_index * 0x4000; // 0x4000 = 16KB
      return rom_data_[bank_offset + address - 0x4000];
    } else {
      u64 bank_index = emu->rom_bank_number;
      u64 bank_offset = bank_index * 0x4000; // 0x4000 = 16KB
      return rom_data_[bank_offset + address - 0x4000];
    }
  }
  if (address >= 0xA000 && address <= 0xBFFF) {
    if (emu->cram_size_) {
      if (!emu->cram_enabled_) { return 0xFF; }
      if (emu->num_rom_banks_ <= 32) {
        if (emu->banking_mode) {
          u64 bank_offset = emu->ram_bank_number * 0x2000; // 0x2000 = 8KB
          return emu->cram_[bank_offset + address - 0xA000];
        } else {
          return emu->cram_[address - 0xA000];
        }
      }
      else{
        return emu->cram_[address - 0xA000];
      }
    }
  }
  std::cerr << "Invalid MBC1 read at address: " << std::hex << address << std::endl;
  return 0xFF;
}

void CART::mbc1_write(EMU* emu, u16 address, u8 value)
{
  if(address <= 0x1FFF){
    if(emu->cram_size_) {
      if((value & 0x0F) == 0x0A){
        emu->cram_enabled_ = true;
      }
      else{
        emu->cram_enabled_ = false;
      }
      return;
    }
  }
  if(address >= 0x2000 && address <= 0x3FFF){
    emu->rom_bank_number = (value & 0x1F);
    if(emu->rom_bank_number == 0){
      emu->rom_bank_number = 1;
    }
    if(emu->num_rom_banks_ <= 2){
      emu->rom_bank_number = emu->rom_bank_number & 0x01;
    }
    else if(emu->num_rom_banks_ <= 4){
      emu->rom_bank_number = emu->rom_bank_number & 0x03;
    }
    else if(emu->num_rom_banks_ <= 8){
      emu->rom_bank_number = emu->rom_bank_number & 0x07;
    }
    else if(emu->num_rom_banks_ <= 16){
      emu->rom_bank_number = emu->rom_bank_number & 0x0F;
    }
    return;
  }
  if(address >= 0x4000 && address <= 0x5FFF){
    emu->ram_bank_number = (value & 0x03);
    if(emu->num_rom_banks_ > 32){
      if (emu->num_rom_banks_ <= 64){
        emu->ram_bank_number = emu->ram_bank_number & 0x01;
      }
    }
    else{
      if (emu->cram_size_ <= 0x2000) {//0x2000 = 8KB
        emu->ram_bank_number = 0;
      } else if (emu->cram_size_ <= 0x4000) {//0x4000 = 16KB
        emu->ram_bank_number &= 0x01;
      }
    }
    return;
  }
  if(address >= 0x6000 && address <= 0x7FFF){
    if (emu->num_rom_banks_ > 32 || emu->cram_size_ > 0x2000) {
      emu->banking_mode = value & 0x01;
    }
    return;
  }
  if(address >= 0xA000 && address <= 0xBFFF){
    if (emu->cram_size_ > 0) {
      if(!emu->cram_enabled_){return;}
      if(emu->num_rom_banks_ <= 32){
        if(emu->banking_mode){
          u64 bank_offset = emu->ram_bank_number * 0x2000; // 0x2000 = 8KB
          if(bank_offset + address - 0xA000 < emu->cram_size_){
            emu->cram_[bank_offset + address - 0xA000] = value;
          }
          else{
            std::cerr << "Invalid write to CRAM at address: " << std::hex << address << std::endl;
          }
        }
        else{
          emu->cram_[address - 0xA000] = value;
        }
      }
      else{
        emu->cram_[address - 0xA000] = value;
      }
      return;
    }
  }
  std::cerr << "Invalid MBC1 write at address: " << std::hex << address << std::endl;
}

u8 CART::mbc2_read(EMU* emu, u16 address)
{
  if (address <= 0x3FFF) {
    return rom_data_[address];
  }
  if (address >= 0x4000 && address <= 0x7FFF) {
    // Cartridge ROM bank 01-0F.
    u64 bank_index = emu->rom_bank_number;
    u64 bank_offset = bank_index * 0x4000;//16KB = 0x4000
    return rom_data_[bank_offset + (address - 0x4000)];
  }
  if (address >= 0xA000 && address <= 0xBFFF) {
    if (!emu->cram_enabled_)
      return 0xFF;
    u16 data_offset = address - 0xA000;
    data_offset %= 512;
    return (emu->cram_[data_offset] & 0x0F) | 0xF0;
  }
  std::cerr << "Invalid MBC2 read at address: " << std::hex << address << std::endl;
  return 0xFF;
}

void CART::mbc2_write(EMU* emu, u16 address, u8 value)
{
  if (address <= 0x3FFF) {
    if (address & 0x100) // bit 8 is set.
    {
      // Set ROM bank number.
      emu->rom_bank_number = value & 0x0F;
      if (emu->rom_bank_number == 0) {
        emu->rom_bank_number = 1;
      }
      if (emu->num_rom_banks_ <= 2) {
        emu->rom_bank_number = emu->rom_bank_number & 0x01;
      } else if (emu->num_rom_banks_ <= 4) {
        emu->rom_bank_number = emu->rom_bank_number & 0x03;
      } else if (emu->num_rom_banks_ <= 8) {
        emu->rom_bank_number = emu->rom_bank_number & 0x07;
      }
      return;
    } else {
      // Enable/disable cartridge RAM.
      if (emu->cram_size_ > 0) {
        if (value == 0x0A) {
          emu->cram_enabled_ = true;
        } else {
          emu->cram_enabled_ = false;
        }
        return;
      }
    }
  } else if (address >= 0xA000 && address <= 0xBFFF) {
    if (!emu->cram_enabled_)
      return;
    u16 data_offset = address - 0xA000;
    data_offset %= 512;
    emu->cram_[data_offset] = value & 0x0F;
    return;
  }
std::cerr << "Invalid MBC2 write at address: " << std::hex << address << std::endl;
  return;
}

u8 CART::mbc3_read(EMU *emu, u16 address) {
  if (address <= 0x3FFF) {
    return rom_data_[address];
  }
  if (address >= 0x4000 && address <= 0x7FFF) {
    // Cartridge ROM bank 01-7F.
    u64 bank_index = emu->rom_bank_number;
    u64 bank_offset = bank_index * 0x4000; // 16KB = 0x4000
    return rom_data_[bank_offset + (address - 0x4000)];
  }
  if (address >= 0xA000 && address <= 0xBFFF) {
    if (emu->ram_bank_number <= 0x03) {
      if (emu->cram_size_) {
        if (!emu->cram_enabled_)
          return 0xFF;
        u64 bank_offset = emu->ram_bank_number * 0x2000; // 0x2000 = 8KB
        // luassert(bank_offset + (address - 0xA000) <= emu->cram_size_);
        if (bank_offset + (address - 0xA000) <= emu->cram_size_) {
          return emu->cram_[bank_offset + (address - 0xA000)];
        }
      }
    }
    if (is_cart_timer(header_->type) && emu->ram_bank_number >= 0x08 &&
        emu->ram_bank_number <= 0x0C) {
      return ((u8 *)(&emu->rtc_.s))[emu->ram_bank_number - 0x08];
    }
  }
  std::cerr << "Invalid MBC3 read at address: " << std::hex << address
            << std::endl;
  return 0xFF;
}

void CART::mbc3_write(EMU* emu, u16 addr, u8 data)
{
  if (addr <= 0x1FFF) {
    // Enable/disable cartridge RAM.
    if (data == 0x0A) {
      emu->cram_enabled_ = true;
    } else {
      emu->cram_enabled_ = false;
    }
    return;
  }
  if (addr >= 0x2000 && addr <= 0x3FFF) {
    // Set ROM bank number.
    emu->rom_bank_number = data & 0x7F;
    if (emu->rom_bank_number == 0) {
      emu->rom_bank_number = 1;
    }
    return;
  }
  if (addr >= 0x4000 && addr <= 0x5FFF) {
    // Set RAM bank number, or map RTC registers.
    emu->ram_bank_number = data;
    return;
  }
  if (addr >= 0x6000 && addr <= 0x7FFF) {
    if (is_cart_timer(header_->type)) {
      if (data == 0x01 && emu->rtc_.time_latching) {
        emu->rtc_.latch();
      }
      if (data == 0x00) {
        emu->rtc_.time_latching = true;
      } else {
        emu->rtc_.time_latching = false;
      }
      return;
    }
  }
  if (addr >= 0xA000 && addr <= 0xBFFF) {
    if (emu->ram_bank_number <= 0x03) {
      if (emu->cram_size_) {
        if (!emu->cram_enabled_)
          return;
        u64 bank_offset = emu->ram_bank_number * 0x2000; // 0x2000 = 8KB
        // luassert(bank_offset + (addr - 0xA000) <= emu->cram_size_);
        if (bank_offset + (addr - 0xA000) < emu->cram_size_) {
          emu->cram_[bank_offset + (addr - 0xA000)] = data;
        }
        return;
      }
    }
    if (is_cart_timer(header_->type) &&
        emu->ram_bank_number >= 0x08 && emu->ram_bank_number <= 0x0C) {
      ((u8 *)(&emu->rtc_))[emu->ram_bank_number - 0x08] = data;
      emu->rtc_.update_timestamp();
      return;
    }
  }
std::cerr << "Invalid MBC3 write at address: " << std::hex << addr << std::endl;
  return;
}

u8 CART::mbc5_read(EMU* emu, u16 address)
{
  if(address <= 0x3FFF) {
    return rom_data_[address];
  }
  if(address >= 0x4000 && address <= 0x7FFF) {
    u64 bank_index = emu->rom_bank_number;
    u64 bank_offset = bank_index * 0x4000;// 16KB = 0x4000
    return rom_data_[bank_offset + (address - 0x4000)];
  }
  if(address >= 0xA000 && address <= 0xBFFF) {
    if(emu->ram_bank_number <= 0x03) {
      if(emu->cram_size_) {
        if(!emu->cram_enabled_)
          return 0xFF;
        u64 bank_offset = emu->ram_bank_number * 0x2000;// 8KB = 0x2000
        if(bank_offset + (address - 0xA000) <= emu->cram_size_) {
          return emu->cram_[bank_offset + (address - 0xA000)];
        }
      }
    }
  }
  std::cerr << "Invalid MBC5 read at address: " << std::hex << address << std::endl;
  return 0xFF;
}

void CART::mbc5_write(EMU* emu, u16 addr, u8 data)
{
  if(addr <= 0x1FFF) {
    // Enable/disable cartridge RAM.
    if(data == 0x0A) {
      emu->cram_enabled_ = true;
    } else {
      emu->cram_enabled_ = false;
    }
    return;
  }
  if(addr >= 0x2000 && addr <= 0x2FFF) {
    // Set ROM bank number. low 8 bit
    emu->rom_bank_number = (emu->rom_bank_number & 0x0100) | (u16)data;
    return;
  }
  if(addr >= 0x3000 && addr <= 0x3FFF) {
    // Set ROM bank number. 9 bit
    emu->rom_bank_number = (emu->rom_bank_number & 0x00FF) | ((u16)(data & 0x01) << 8);
    return;
  }
  if(addr >= 0x4000 && addr <= 0x5FFF) {
    emu->ram_bank_number = data & 0x0F;
    return;
  }
  if(addr >= 0xA000 && addr <= 0xBFFF) {
    if(emu->cram_size_) {
      if(!emu->cram_enabled_) return;
      u64 bank_offset = emu->ram_bank_number * 0x2000; // 0x2000 = 8KB
      if(bank_offset + (addr - 0xA000) < emu->cram_size_) {
        emu->cram_[bank_offset + (addr - 0xA000)] = data;
        return;
      }
    }
  }
  std::cerr << "Invalid MBC5 write at address: " << std::hex << addr << std::endl;
  return;
}
