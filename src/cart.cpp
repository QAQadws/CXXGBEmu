#include "cart.h"
#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <cstring>


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
