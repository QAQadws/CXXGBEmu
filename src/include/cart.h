#ifndef __CART_H__
#define __CART_H__

#include "defs.h"
#include <memory>
#include <string>

struct rom_header {
  u8 entry[4];         // 0x00-0x03 (4 bytes)
  u8 logo[0x30];       // 0x04-0x33 (48 bytes)
  char title[16];    // 0x34-0x43 (22 bytes)
  u16 new_lic_code;    // 0x44-0x45 (2 bytes)
  u8 sgb_flag;         // 0x46 (1 byte)
  u8 type;             // 0x47 (1 byte)
  u8 rom_size;         // 0x48 (1 byte)
  u8 ram_size;         // 0x49 (1 byte)
  u8 dest_code;        // 0x4A (1 byte)
  u8 lic_code;         // 0x4B (1 byte)
  u8 version;          // 0x4C (1 byte)
  u8 checksum;         // 0x4D (1 byte)
  u16 global_checksum; // 0x4E-0x4F (2 bytes)
};

class CART {
public:
    bool load_cart(const char* filename);
    std::string cart_type_name() const;
    std::string cart_lic_code() const;

    CART() = default;
    CART(const char* filename) {
        load_cart(filename);
    }

    ~CART() = default;


  private:
    std::unique_ptr<rom_header> header_;
    std::unique_ptr<u8[]> rom_data_;
    u32 rom_size_;
};
#endif // __CART_H__