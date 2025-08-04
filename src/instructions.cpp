#include"instructions.h"
#include "defs.h"
#include "emu.h"
#include<iostream>

inline constexpr u16 make_u16(u8 low, u8 high)
{
    return ((u16)low) | (((u16)high) << 8);
}
//! Reads 16-bit immediate data.
inline u16 read_d16(EMU* emu)
{
    u16 r = make_u16(emu->bus_read(emu->cpu_.pc), emu->bus_read(emu->cpu_.pc + 1));
#ifdef DEBUG
    std::cout << "\t" << std::hex << r;
#endif
    emu->cpu_.pc += 2;
    return r;
}
inline u8 read_d8(EMU* emu)
{
    u8 r = emu->bus_read(emu->cpu_.pc);
#ifdef DEBUG
    std::cout << "\t" << std::hex << r;
#endif
    ++emu->cpu_.pc;
    return r;
}
inline void set_zero_flag(EMU* emu, u8 value)
{
    if (value) {
        emu->cpu_.reset_fz();
    } else {
        emu->cpu_.set_fz();
    }
}
inline void cp_8(EMU *emu, u16 v1, u16 v2) {
  u8 temp = static_cast<u8>(v1) - static_cast<u8>(v2);
  set_zero_flag(emu, temp);
  emu->cpu_.set_fn();
  if ((v1 & 0x0F) < (v2 & 0x0F)) {
    emu->cpu_.set_fh();
  } else {
    emu->cpu_.reset_fh();
  }
  if (v1 < v2) {
    emu->cpu_.set_fc();
  } else {
    emu->cpu_.reset_fc();
  }
}
inline void push_16(EMU *emu, u16 value) {
    emu->cpu_.sp -= 2;
    emu->bus_write(emu->cpu_.sp, static_cast<u8>(value & 0xFF));
    emu->bus_write(emu->cpu_.sp + 1, static_cast<u8>((value >> 8) & 0xFF));
}
inline u16 pop_16(EMU *emu) {
    u16 value = make_u16(emu->bus_read(emu->cpu_.sp), emu->bus_read(emu->cpu_.sp + 1));
    emu->cpu_.sp += 2;
    return value;
}
inline void inc_8(EMU *emu, u8 &v) {
    ++v;
    set_zero_flag(emu, v);
    if((v & 0x0F) == 0x00) {
        emu->cpu_.set_fh();
    } else {
        emu->cpu_.reset_fh();
    }
}
inline void dec_8(EMU *emu, u8 &v) {
    --v;
    set_zero_flag(emu, v);
    emu->cpu_.set_fn();
    if((v & 0x0F) == 0x00) {
        emu->cpu_.set_fh();
    } else {
        emu->cpu_.reset_fh();
    }
}
inline u8 add_8(EMU *emu, u16 v1, u16 v2) {
  u8 result = static_cast<u8>(v1 + v2);
  set_zero_flag(emu, result);
  emu->cpu_.reset_fn();
  if ((v1 & 0x0F) + (v2 & 0x0F) > 0x0F) {
    emu->cpu_.set_fh(); // 设置半进位标志
  } else {
    emu->cpu_.reset_fh(); // 重置半进位标志
  }
  if (v1 + v2 > 0xFF) {
    emu->cpu_.set_fc(); // 设置进位标志
  } else {
    emu->cpu_.reset_fc(); // 重置进位标志
  }
  return result;
}
inline u16 add_16(EMU *emu, u32 v1, u32 v2) {
  emu->cpu_.reset_fn();
  if ((v1 & 0xFFF) + (v2 & 0xFFF) > 0xFFF)
    emu->cpu_.set_fh();
  else
    emu->cpu_.reset_fh();
  if (v1 + v2 > 0xFFFF)
    emu->cpu_.set_fc();
  else
    emu->cpu_.reset_fc();
  return (u16)(v1 + v2);
}
inline u8 adc_8(EMU *emu, u16 v1, u16 v2) {
 u16 c = emu->cpu_.fc() ? 1 : 0; // 获取进位标志
 u8 result = static_cast<u8>(v1 + v2 + c);
 set_zero_flag(emu, result);
 emu->cpu_.reset_fn();
 if (((v1 & 0x0F) + (v2 & 0x0F) + c) > 0x0F) {
     emu->cpu_.set_fh(); // 设置半进位标志
 } else {
     emu->cpu_.reset_fh(); // 重置半进位标志
 }
 if ((v1 + v2 + c )> 0xFF) {
     emu->cpu_.set_fc(); // 设置进位标志
 } else {
     emu->cpu_.reset_fc(); // 重置进位标志
 }
 return result;
}
inline u8 sub_8(EMU *emu, u16 v1, u16 v2) {
    u8 result = static_cast<u8>(v1) - static_cast<u8>(v2);
    set_zero_flag(emu, result);
    emu->cpu_.set_fn();
    if ((v1 & 0x0F) < (v2 & 0x0F)) {
        emu->cpu_.set_fh(); // 设置半进位标志
    } else {
        emu->cpu_.reset_fh(); // 重置半进位标志
    }
    if (v1 < v2) {
        emu->cpu_.set_fc(); // 设置进位标志
    } else {
        emu->cpu_.reset_fc(); // 重置进位标志
    }
    return result;
}
inline u8 sbc_8(EMU *emu, u16 v1, u16 v2) {
    u16 c = emu->cpu_.fc() ? 1 : 0; // 获取进位标志
    u8 result = static_cast<u8>(v1) - static_cast<u8>(v2) - c;
    set_zero_flag(emu, result);
    emu->cpu_.set_fn();
    if ((v1 & 0x0F) < ((v2 & 0x0F) + c)) {
        emu->cpu_.set_fh(); // 设置半进位标志
    } else {
        emu->cpu_.reset_fh(); // 重置半进位标志
    }
    if (v1 < (v2 + c)) {
        emu->cpu_.set_fc(); // 设置进位标志
    } else {
        emu->cpu_.reset_fc(); // 重置进位标志
    }
    return result;
}
inline u8 and_8(EMU *emu, u8 v1, u8 v2) {
    u8 result = v1 & v2;
    set_zero_flag(emu, result);
    emu->cpu_.reset_fn();
    emu->cpu_.set_fh();
    emu->cpu_.reset_fc();
    return result;
}
inline u8 xor_8(EMU *emu, u8 v1, u8 v2) {
    u8 result = v1 ^ v2;
    set_zero_flag(emu, result);
    emu->cpu_.reset_fn();
    emu->cpu_.reset_fh();
    emu->cpu_.reset_fc();
    return result;
}
inline u8 or_8(EMU *emu, u8 v1, u8 v2) {
    u8 result = v1 | v2;
    set_zero_flag(emu, result);
    emu->cpu_.reset_fn();
    emu->cpu_.reset_fh();
    emu->cpu_.reset_fc();
    return result;
}
inline void rlc_8(EMU *emu, u8 &v) {
    u8 carry = v >> 7; // 获取最高位
    v = (v << 1) | carry; // 左移并将最高位放到最低位
    set_zero_flag(emu, v);
    emu->cpu_.reset_fn();
    emu->cpu_.reset_fh();
    if (carry) {
        emu->cpu_.set_fc(); // 设置进位标志
    } else {
        emu->cpu_.reset_fc(); // 重置进位标志
    }
}
inline void rl_8(EMU *emu, u8 &v) {
    u8 carry = v >> 7; // 获取最高位
    v = (v << 1) | emu->cpu_.fc(); // 左移并将进位标志放到最低位
    set_zero_flag(emu, v);
    emu->cpu_.reset_fn();
    emu->cpu_.reset_fh();
    if (carry) {
        emu->cpu_.set_fc(); // 设置进位标志
    } else {
        emu->cpu_.reset_fc(); // 重置进位标志
    }
}
inline void rrc_8(EMU *emu, u8 &v) {
    u8 carry = v & 0x01; // 获取最低位
    v = (v >> 1) | (carry << 7); // 右移并将最低位放到最高位
    set_zero_flag(emu, v);
    emu->cpu_.reset_fn();
    emu->cpu_.reset_fh();
    if (carry) {
        emu->cpu_.set_fc(); // 设置进位标志
    } else {
        emu->cpu_.reset_fc(); // 重置进位标志
    }
}
inline void rr_8(EMU *emu, u8 &v) {
    u8 carry = v & 0x01; // 获取最低位
    v = (v >> 1) | (emu->cpu_.fc() << 7); // 右移并将进位标志放到最高位
    set_zero_flag(emu, v);
    emu->cpu_.reset_fn();
    emu->cpu_.reset_fh();
    if (carry) {
        emu->cpu_.set_fc(); // 设置进位标志
    } else {
        emu->cpu_.reset_fc(); // 重置进位标志
    }
}
inline void sla_8(EMU *emu, u8 &v) {
    u8 carry = v >> 7; // 获取最高位
    v <<= 1; // 左移
    set_zero_flag(emu, v);
    emu->cpu_.reset_fn();
    emu->cpu_.reset_fh();
    if (carry) {
        emu->cpu_.set_fc(); // 设置进位标志
    } else {
        emu->cpu_.reset_fc(); // 重置进位标志
    }
}
inline void sra_8(EMU *emu, u8 &v) {
    u8 carry = v & 0x01; // 获取最低位
    v = (v >> 1) | (v & 0x80); // 右移并保持符号位
    set_zero_flag(emu, v);
    emu->cpu_.reset_fn();
    emu->cpu_.reset_fh();
    if (carry) {
        emu->cpu_.set_fc(); // 设置进位标志
    } else {
        emu->cpu_.reset_fc(); // 重置进位标志
    }
}
inline void srl_8(EMU *emu, u8 &v) {
    u8 carry = v & 0x01; // 获取最低位
    v >>= 1; // 右移
    set_zero_flag(emu, v);
    emu->cpu_.reset_fn();
    emu->cpu_.reset_fh();
    if (carry) {
        emu->cpu_.set_fc(); // 设置进位标志
    } else {
        emu->cpu_.reset_fc(); // 重置进位标志
    }
}
inline void swap_8(EMU *emu, u8 &v) {
    v = (v << 4) | (v >> 4); // 交换高四位和低四位
    set_zero_flag(emu, v);
    emu->cpu_.reset_fn();
    emu->cpu_.reset_fh();
    emu->cpu_.reset_fc(); // 交换后不设置进位标志
}
inline void bit_8(EMU *emu, u8 v, u8 bit) {
    if (v & (1 << bit)) {
        emu->cpu_.reset_fz(); // 如果对应位为1，清除零标志
    } else {
        emu->cpu_.set_fz(); // 如果对应位为0，设置零标志
    }
    emu->cpu_.reset_fn(); // 清除负标志
    emu->cpu_.set_fh(); // 设置半进位标志
}
inline void res_8(EMU *emu, u8 &v, u8 bit) {
    v &= ~(1 << bit); // 清除指定的位
}
inline void set_8(EMU *emu, u8 &v, u8 bit) {
    v |= (1 << bit); // 设置指定的位
}
// NOP instruction does nothing
void x00_nop(EMU *emu) {
    emu->tick(1);
}
// LD BC,nn instruction loads a 16-bit immediate value into the BC register pair
void x01_ld_bc_d16(EMU *emu) {
    emu->cpu_.bc(read_d16(emu));
    emu->tick(3);
}
void x02_ld_mbc_a(EMU *emu) {
    emu->bus_write(emu->cpu_.bc(), emu->cpu_.a);
    emu->tick(2);
}
void x03_inc_bc(EMU *emu) {
    emu->cpu_.bc(emu->cpu_.bc() + 1);
    emu->tick(2);
}
void x04_inc_b(EMU *emu) {
    inc_8(emu, emu->cpu_.b);
    emu->tick(1);
}
void x05_dec_b(EMU *emu) {
    dec_8(emu, emu->cpu_.b);
    emu->tick(1);
}
void x06_ld_b_d8(EMU *emu) {
    emu->cpu_.b = read_d8(emu);
    emu->tick(2);
}
void x07_rlca(EMU *emu) {
    rlc_8(emu, emu->cpu_.a);
    emu->cpu_.reset_fz();
    emu->tick(1);
}
// LD (BC),A instruction stores the value in the A register into the memory location pointed to by BC
void x08_ld_a16_sp(EMU *emu) {
    u16 address = read_d16(emu);
    emu->bus_write(address, emu->cpu_.sp & 0xFF);
    emu->bus_write(address + 1, (emu->cpu_.sp >> 8) & 0xFF);
    emu->tick(5);
}
void x09_add_hl_bc(EMU *emu) {
    emu->cpu_.hl(add_16(emu, emu->cpu_.hl(), emu->cpu_.bc()));
    emu->tick(2);
}
void x0a_ld_a_mbc(EMU *emu) {
    emu->cpu_.a = emu->bus_read(emu->cpu_.bc());
    emu->tick(2);
}
void x0b_dec_bc(EMU *emu) {
    emu->cpu_.bc(emu->cpu_.bc() - 1);
    emu->tick(2);
}
void x0c_inc_c(EMU *emu) {
    inc_8(emu, emu->cpu_.c);
    emu->tick(1);
}
void x0d_dec_c(EMU *emu) {
    dec_8(emu, emu->cpu_.c);
    emu->tick(1);
}
void x0e_ld_c_d8(EMU *emu) {
    emu->cpu_.c = read_d8(emu);
    emu->tick(2);
}
void x0f_rrca(EMU *emu) {
    rrc_8(emu, emu->cpu_.a);
    emu->cpu_.reset_fz();
    emu->tick(1);
}
void x10_stop(EMU *emu) {
    read_d8(emu);
    emu->paused_ = true; // 停止执行
    emu->tick(1);
}
void x11_ld_de_d16(EMU *emu) {
    emu->cpu_.de(read_d16(emu));
    emu->tick(3);
}
void x12_ld_mde_a(EMU *emu) {
    emu->bus_write(emu->cpu_.de(), emu->cpu_.a);
    emu->tick(2);
}
void x13_inc_de(EMU *emu) {
    emu->cpu_.de(emu->cpu_.de() + 1);
    emu->tick(2);
}
void x14_inc_d(EMU *emu) {
    inc_8(emu, emu->cpu_.d);
    emu->tick(1);
}
void x15_dec_d(EMU *emu) {
    dec_8(emu, emu->cpu_.d);
    emu->tick(1);
}
void x16_ld_d_d8(EMU *emu) {
    emu->cpu_.d = read_d8(emu);
    emu->tick(2);
}
void x17_rla(EMU *emu) {
    rl_8(emu, emu->cpu_.a);
    emu->cpu_.reset_fz();
    emu->tick(1);
}
void x18_jr_r8(EMU *emu) {
    s8 offset = (s8)read_d8(emu);
    emu->cpu_.pc += offset;
    emu->tick(3);
}
void x19_add_hl_de(EMU *emu) {
    emu->cpu_.hl(add_16(emu, emu->cpu_.hl(), emu->cpu_.de()));
    emu->tick(2);
}
void x1a_ld_a_mde(EMU *emu) {
    emu->cpu_.a = emu->bus_read(emu->cpu_.de());
    emu->tick(2);
}
void x1b_dec_de(EMU *emu) {
    emu->cpu_.de(emu->cpu_.de() - 1);
    emu->tick(2);
}
void x1c_inc_e(EMU *emu) {
    inc_8(emu, emu->cpu_.e);
    emu->tick(1);
}
void x1d_dec_e(EMU *emu) {
    dec_8(emu, emu->cpu_.e);
    emu->tick(1);
}
void x1e_ld_e_d8(EMU *emu) {
    emu->cpu_.e = read_d8(emu);
    emu->tick(2);
}
void x1f_rra(EMU *emu) {
    rr_8(emu, emu->cpu_.a);
    emu->cpu_.reset_fz();
    emu->tick(1);
}
void x20_jr_nz_r8(EMU *emu) {
    s8 offset = (s8)read_d8(emu);
    if (!emu->cpu_.fz()) { // 如果零标志位为0
        emu->cpu_.pc += offset;
        emu->tick(3);
    } else {
        emu->tick(2);
    }
}
void x21_ld_hl_d16(EMU *emu) {
    emu->cpu_.hl(read_d16(emu));
    emu->tick(3);
}
// LD (HL+),A instruction stores the value in the A register into the memory location pointed to by HL, and then increments HL
void x22_ldi_mhl_a(EMU *emu) {
    u16 address = emu->cpu_.hl();
    emu->bus_write(address, emu->cpu_.a);
    emu->cpu_.hl(address + 1); // HL自增
    emu->tick(2);
}
void x23_inc_hl(EMU *emu) {
    emu->cpu_.hl(emu->cpu_.hl() + 1);
    emu->tick(2);
}
void x24_inc_h(EMU *emu) {
    inc_8(emu, emu->cpu_.h);
    emu->tick(1);
}
void x25_dec_h(EMU *emu) {
    dec_8(emu, emu->cpu_.h);
    emu->tick(1);
}
void x26_ld_h_d8(EMU *emu) {
    emu->cpu_.h = read_d8(emu);
    emu->tick(2);
}
void x27_daa(EMU *emu) {
  if (emu->cpu_.fn()) {
    if (emu->cpu_.fc()) {
      if (emu->cpu_.fh())
        emu->cpu_.a += 0x9A;
      else
        emu->cpu_.a += 0xA0;
    } else {
      if (emu->cpu_.fh())
        emu->cpu_.a += 0xFA;
    }
  } else {
    if (emu->cpu_.fc() || (emu->cpu_.a > 0x99)) {
      if (emu->cpu_.fh() || ((emu->cpu_.a & 0x0F) > 0x09))
        emu->cpu_.a += 0x66;
      else
        emu->cpu_.a += 0x60;
      emu->cpu_.set_fc();
    } else {
      if (emu->cpu_.fh() || ((emu->cpu_.a & 0x0F) > 0x09))
        emu->cpu_.a += 0x06;
    }
  }
  set_zero_flag(emu, emu->cpu_.a);
  emu->cpu_.reset_fh();
  emu->tick(1);
}
void x28_jr_z_r8(EMU *emu) {
    s8 offset = (s8)read_d8(emu);
    if (emu->cpu_.fz()) { // 如果零标志位为1
        emu->cpu_.pc += offset;
        emu->tick(3);
    } else {
        emu->tick(2);
    }
}
void x29_add_hl_hl(EMU *emu) {
    emu->cpu_.hl(add_16(emu, emu->cpu_.hl(), emu->cpu_.hl()));
    emu->tick(2);
}
//LD A,(HL+) instruction loads the value from the memory location pointed to by HL into the A register, and then increments HL
void x2a_ldi_a_mhl(EMU *emu) {
    u16 address = emu->cpu_.hl();
    emu->cpu_.a = emu->bus_read(address);
    emu->cpu_.hl(address + 1); // HL自增
    emu->tick(2);
}
void x2b_dec_hl(EMU *emu) {
    emu->cpu_.hl(emu->cpu_.hl() - 1);
    emu->tick(2);
}
void x2c_inc_l(EMU *emu) {
    inc_8(emu, emu->cpu_.l);
    emu->tick(1);
}
void x2d_dec_l(EMU *emu) {
    dec_8(emu, emu->cpu_.l);
    emu->tick(1);
}
void x2e_ld_l_d8(EMU *emu) {
    emu->cpu_.l = read_d8(emu);
    emu->tick(2);
}
void x2f_cpl(EMU *emu) {
    emu->cpu_.a = ~emu->cpu_.a; // 取反A寄存器的值
    emu->cpu_.set_fh(); // 设置半进位标志
    emu->cpu_.set_fn(); // 设置负标志
    emu->tick(1);
}
void x30_jr_nc_r8(EMU *emu) {
    s8 offset = (s8)read_d8(emu);
    if (!emu->cpu_.fc()) { // 如果进位标志位为0
        emu->cpu_.pc += offset;
        emu->tick(3);
    } else {
        emu->tick(2);
    }
}
void x31_ld_sp_d16(EMU *emu) {
    emu->cpu_.sp = read_d16(emu);
    emu->tick(3);
}
// LD (HL-),A instruction stores the value in the A register into the memory location pointed to by HL, and then decrements HL
void x32_ldd_mhl_a(EMU *emu) {
    u16 address = emu->cpu_.hl();
    emu->bus_write(address, emu->cpu_.a);
    emu->cpu_.hl(address - 1); // HL自减
    emu->tick(2);
}
void x33_inc_sp(EMU *emu) {
    ++emu->cpu_.sp;
    emu->tick(2);
}
void x34_inc_mhl(EMU *emu) {
    u8 temp = emu->bus_read(emu->cpu_.hl());
    inc_8(emu, temp);
    emu->bus_write(emu->cpu_.hl(), temp);
    emu->tick(3);
}
void x35_dec_mhl(EMU *emu) {
    u8 temp = emu->bus_read(emu->cpu_.hl());
    dec_8(emu, temp);
    emu->bus_write(emu->cpu_.hl(), temp);
    emu->tick(3);
}
void x36_ld_mhl_d8(EMU *emu) {
    emu->bus_write(emu->cpu_.hl(), read_d8(emu));
    emu->tick(3);
}
void x37_scf(EMU *emu) {
    emu->cpu_.reset_fn(); // 清除负标志
    emu->cpu_.reset_fh(); // 清除半进位标志
    emu->cpu_.set_fc();   // 设置进位标志
    emu->tick(1);
}
void x38_jr_c_r8(EMU *emu) {
    s8 offset = (s8)read_d8(emu);
    if (emu->cpu_.fc()) { // 如果进位标志位为1
        emu->cpu_.pc += offset;
        emu->tick(3);
    } else {
        emu->tick(2);
    }
}
void x39_add_hl_sp(EMU *emu) {
    emu->cpu_.hl(add_16(emu, emu->cpu_.hl(), emu->cpu_.sp));
    emu->tick(2);
}
// LD A,(HL-) instruction loads the value from the memory location pointed to by HL into the A register, and then decrements HL
void x3a_ldd_a_mhl(EMU *emu) {
    u16 address = emu->cpu_.hl();
    emu->cpu_.a = emu->bus_read(address);
    emu->cpu_.hl(address - 1); // HL自减
    emu->tick(2);
}
void x3b_dec_sp(EMU *emu) {
    --emu->cpu_.sp;
    emu->tick(2);
}
void x3c_inc_a(EMU *emu) {
    inc_8(emu, emu->cpu_.a);
    emu->tick(1);
}
void x3d_dec_a(EMU *emu) {
    dec_8(emu, emu->cpu_.a);
    emu->tick(1);
}
void x3e_ld_a_d8(EMU *emu) {
    emu->cpu_.a = read_d8(emu);
    emu->tick(2);
}
void x3f_ccf(EMU *emu) {
    emu->cpu_.reset_fn(); // 清除负标志
    emu->cpu_.reset_fh(); // 清除半进位标志
    if(emu->cpu_.fc()) {
        emu->cpu_.reset_fc(); // 如果进位标志为1，则清除它
    } else {
        emu->cpu_.set_fc();   // 如果进位标志为0，则设置它
    }
    emu->tick(1);
}
void x40_ld_b_b(EMU *emu) {
    emu->cpu_.b = emu->cpu_.b;
    emu->tick(1);
}
void x41_ld_b_c(EMU *emu) {
    emu->cpu_.b = emu->cpu_.c;
    emu->tick(1);
}
void x42_ld_b_d(EMU *emu) {
    emu->cpu_.b = emu->cpu_.d;
    emu->tick(1);
}
void x43_ld_b_e(EMU *emu) {
    emu->cpu_.b = emu->cpu_.e;
    emu->tick(1);
}
void x44_ld_b_h(EMU *emu) {
    emu->cpu_.b = emu->cpu_.h;
    emu->tick(1);
}
void x45_ld_b_l(EMU *emu) {
    emu->cpu_.b = emu->cpu_.l;
    emu->tick(1);
}
void x46_ld_b_mhl(EMU *emu) {
    emu->cpu_.b = emu->bus_read(emu->cpu_.hl());
    emu->tick(2);
}
void x47_ld_b_a(EMU *emu) {
    emu->cpu_.b = emu->cpu_.a;
    emu->tick(1);
}
void x48_ld_c_b(EMU *emu) {
    emu->cpu_.c = emu->cpu_.b;
    emu->tick(1);
}
void x49_ld_c_c(EMU *emu) {
    emu->cpu_.c = emu->cpu_.c;
    emu->tick(1);
}
void x4a_ld_c_d(EMU *emu) {
    emu->cpu_.c = emu->cpu_.d;
    emu->tick(1);
}
void x4b_ld_c_e(EMU *emu) {
    emu->cpu_.c = emu->cpu_.e;
    emu->tick(1);
}
void x4c_ld_c_h(EMU *emu) {
    emu->cpu_.c = emu->cpu_.h;
    emu->tick(1);
}
void x4d_ld_c_l(EMU *emu) {
    emu->cpu_.c = emu->cpu_.l;
    emu->tick(1);
}
void x4e_ld_c_mhl(EMU *emu) {
    emu->cpu_.c = emu->bus_read(emu->cpu_.hl());
    emu->tick(2);
}
void x4f_ld_c_a(EMU *emu) {
    emu->cpu_.c = emu->cpu_.a;
    emu->tick(1);
}
void x50_ld_d_b(EMU *emu) {
    emu->cpu_.d = emu->cpu_.b;
    emu->tick(1);
}
void x51_ld_d_c(EMU *emu) {
    emu->cpu_.d = emu->cpu_.c;
    emu->tick(1);
}
void x52_ld_d_d(EMU *emu) {
    emu->cpu_.d = emu->cpu_.d;
    emu->tick(1);
}
void x53_ld_d_e(EMU *emu) {
    emu->cpu_.d = emu->cpu_.e;
    emu->tick(1);
}
void x54_ld_d_h(EMU *emu) {
    emu->cpu_.d = emu->cpu_.h;
    emu->tick(1);
}
void x55_ld_d_l(EMU *emu) {
    emu->cpu_.d = emu->cpu_.l;
    emu->tick(1);
}
void x56_ld_d_mhl(EMU *emu) {
    emu->cpu_.d = emu->bus_read(emu->cpu_.hl());
    emu->tick(2);
}
void x57_ld_d_a(EMU *emu) {
    emu->cpu_.d = emu->cpu_.a;
    emu->tick(1);
}
void x58_ld_e_b(EMU *emu) {
    emu->cpu_.e = emu->cpu_.b;
    emu->tick(1);
}
void x59_ld_e_c(EMU *emu) {
    emu->cpu_.e = emu->cpu_.c;
    emu->tick(1);
}
void x5a_ld_e_d(EMU *emu) {
    emu->cpu_.e = emu->cpu_.d;
    emu->tick(1);
}
void x5b_ld_e_e(EMU *emu) {
    emu->cpu_.e = emu->cpu_.e;
    emu->tick(1);
}
void x5c_ld_e_h(EMU *emu) {
    emu->cpu_.e = emu->cpu_.h;
    emu->tick(1);
}
void x5d_ld_e_l(EMU *emu) {
    emu->cpu_.e = emu->cpu_.l;
    emu->tick(1);
}
void x5e_ld_e_mhl(EMU *emu) {
    emu->cpu_.e = emu->bus_read(emu->cpu_.hl());
    emu->tick(2);
}
void x5f_ld_e_a(EMU *emu) {
    emu->cpu_.e = emu->cpu_.a;
    emu->tick(1);
}
void x60_ld_h_b(EMU *emu) {
    emu->cpu_.h = emu->cpu_.b;
    emu->tick(1);
}
void x61_ld_h_c(EMU *emu) {
    emu->cpu_.h = emu->cpu_.c;
    emu->tick(1);
}
void x62_ld_h_d(EMU *emu) {
    emu->cpu_.h = emu->cpu_.d;
    emu->tick(1);
}
void x63_ld_h_e(EMU *emu) {
    emu->cpu_.h = emu->cpu_.e;
    emu->tick(1);
}
void x64_ld_h_h(EMU *emu) {
    emu->cpu_.h = emu->cpu_.h;
    emu->tick(1);
}
void x65_ld_h_l(EMU *emu) {
    emu->cpu_.h = emu->cpu_.l;
    emu->tick(1);
}
void x66_ld_h_mhl(EMU *emu) {
    emu->cpu_.h = emu->bus_read(emu->cpu_.hl());
    emu->tick(2);
}
void x67_ld_h_a(EMU *emu) {
    emu->cpu_.h = emu->cpu_.a;
    emu->tick(1);
}
void x68_ld_l_b(EMU *emu) {
    emu->cpu_.l = emu->cpu_.b;
    emu->tick(1);
}
void x69_ld_l_c(EMU *emu) {
    emu->cpu_.l = emu->cpu_.c;
    emu->tick(1);
}
void x6a_ld_l_d(EMU *emu) {
    emu->cpu_.l = emu->cpu_.d;
    emu->tick(1);
}
void x6b_ld_l_e(EMU *emu) {
    emu->cpu_.l = emu->cpu_.e;
    emu->tick(1);
}
void x6c_ld_l_h(EMU *emu) {
    emu->cpu_.l = emu->cpu_.h;
    emu->tick(1);
}
void x6d_ld_l_l(EMU *emu) {
    emu->cpu_.l = emu->cpu_.l;
    emu->tick(1);
}
void x6e_ld_l_mhl(EMU *emu) {
    emu->cpu_.l = emu->bus_read(emu->cpu_.hl());
    emu->tick(2);
}
void x6f_ld_l_a(EMU *emu) {
    emu->cpu_.l = emu->cpu_.a;
    emu->tick(1);
}
void x70_ld_mhl_b(EMU *emu) {
    emu->bus_write(emu->cpu_.hl(), emu->cpu_.b);
    emu->tick(2);
}
void x71_ld_mhl_c(EMU *emu) {
    emu->bus_write(emu->cpu_.hl(), emu->cpu_.c);
    emu->tick(2);
}
void x72_ld_mhl_d(EMU *emu) {
    emu->bus_write(emu->cpu_.hl(), emu->cpu_.d);
    emu->tick(2);
}
void x73_ld_mhl_e(EMU *emu) {
    emu->bus_write(emu->cpu_.hl(), emu->cpu_.e);
    emu->tick(2);
}
void x74_ld_mhl_h(EMU *emu) {
    emu->bus_write(emu->cpu_.hl(), emu->cpu_.h);
    emu->tick(2);
}
void x75_ld_mhl_l(EMU *emu) {
    emu->bus_write(emu->cpu_.hl(), emu->cpu_.l);
    emu->tick(2);
}
void x76_halt(EMU *emu) {
    emu->cpu_.halted_ = true;
    emu->tick(1);
}
void x77_ld_mhl_a(EMU *emu) {
    emu->bus_write(emu->cpu_.hl(), emu->cpu_.a);
    emu->tick(2);
}
void x78_ld_a_b(EMU *emu) {
    emu->cpu_.a = emu->cpu_.b;
    emu->tick(1);
}
void x79_ld_a_c(EMU *emu) {
    emu->cpu_.a = emu->cpu_.c;
    emu->tick(1);
}
void x7a_ld_a_d(EMU *emu) {
    emu->cpu_.a = emu->cpu_.d;
    emu->tick(1);
}
void x7b_ld_a_e(EMU *emu) {
    emu->cpu_.a = emu->cpu_.e;
    emu->tick(1);
}
void x7c_ld_a_h(EMU *emu) {
    emu->cpu_.a = emu->cpu_.h;
    emu->tick(1);
}
void x7d_ld_a_l(EMU *emu) {
    emu->cpu_.a = emu->cpu_.l;
    emu->tick(1);
}
void x7e_ld_a_mhl(EMU *emu) {
    emu->cpu_.a = emu->bus_read(emu->cpu_.hl());
    emu->tick(2);
}
void x7f_ld_a_a(EMU *emu) {
    emu->cpu_.a = emu->cpu_.a;
    emu->tick(1);
}
void x80_add_a_b(EMU *emu) {
    emu->cpu_.a = add_8(emu, emu->cpu_.a, emu->cpu_.b);
    emu->tick(1);
}
void x81_add_a_c(EMU *emu) {
    emu->cpu_.a = add_8(emu, emu->cpu_.a, emu->cpu_.c);
    emu->tick(1);
}
void x82_add_a_d(EMU *emu) {
    emu->cpu_.a = add_8(emu, emu->cpu_.a, emu->cpu_.d);
    emu->tick(1);
}
void x83_add_a_e(EMU *emu) {
    emu->cpu_.a = add_8(emu, emu->cpu_.a, emu->cpu_.e);
    emu->tick(1);
}
void x84_add_a_h(EMU *emu) {
    emu->cpu_.a = add_8(emu, emu->cpu_.a, emu->cpu_.h);
    emu->tick(1);
}
void x85_add_a_l(EMU *emu) {
    emu->cpu_.a = add_8(emu, emu->cpu_.a, emu->cpu_.l);
    emu->tick(1);
}
void x86_add_a_mhl(EMU *emu) {
    emu->cpu_.a = add_8(emu, emu->cpu_.a, emu->bus_read(emu->cpu_.hl()));
    emu->tick(2);
}
void x87_add_a_a(EMU *emu) {
    emu->cpu_.a = add_8(emu, emu->cpu_.a, emu->cpu_.a);
    emu->tick(1);
}
void x88_adc_a_b(EMU *emu) {
    emu->cpu_.a = adc_8(emu, emu->cpu_.a, emu->cpu_.b);
    emu->tick(1);
}
void x89_adc_a_c(EMU *emu) {
    emu->cpu_.a = adc_8(emu, emu->cpu_.a, emu->cpu_.c);
    emu->tick(1);
}
void x8a_adc_a_d(EMU *emu) {
    emu->cpu_.a = adc_8(emu, emu->cpu_.a, emu->cpu_.d);
    emu->tick(1);
}
void x8b_adc_a_e(EMU *emu) {
    emu->cpu_.a = adc_8(emu, emu->cpu_.a, emu->cpu_.e);
    emu->tick(1);
}
void x8c_adc_a_h(EMU *emu) {
    emu->cpu_.a = adc_8(emu, emu->cpu_.a, emu->cpu_.h);
    emu->tick(1);
}
void x8d_adc_a_l(EMU *emu) {
    emu->cpu_.a = adc_8(emu, emu->cpu_.a, emu->cpu_.l);
    emu->tick(1);
}
void x8e_adc_a_mhl(EMU *emu) {
    emu->cpu_.a = adc_8(emu, emu->cpu_.a, emu->bus_read(emu->cpu_.hl()));
    emu->tick(2);
}
void x8f_adc_a_a(EMU *emu) {
    emu->cpu_.a = adc_8(emu, emu->cpu_.a, emu->cpu_.a);
    emu->tick(1);
}
void x90_sub_b(EMU *emu) {
    emu->cpu_.a = sub_8(emu, emu->cpu_.a, emu->cpu_.b);
    emu->tick(1);
}
void x91_sub_c(EMU *emu) {
    emu->cpu_.a = sub_8(emu, emu->cpu_.a, emu->cpu_.c);
    emu->tick(1);
}
void x92_sub_d(EMU *emu) {
    emu->cpu_.a = sub_8(emu, emu->cpu_.a, emu->cpu_.d);
    emu->tick(1);
}
void x93_sub_e(EMU *emu) {
    emu->cpu_.a = sub_8(emu, emu->cpu_.a, emu->cpu_.e);
    emu->tick(1);
}
void x94_sub_h(EMU *emu) {
    emu->cpu_.a = sub_8(emu, emu->cpu_.a, emu->cpu_.h);
    emu->tick(1);
}
void x95_sub_l(EMU *emu) {
    emu->cpu_.a = sub_8(emu, emu->cpu_.a, emu->cpu_.l);
    emu->tick(1);
}
void x96_sub_mhl(EMU *emu) {
    emu->cpu_.a = sub_8(emu, emu->cpu_.a, emu->bus_read(emu->cpu_.hl()));
    emu->tick(2);
}
void x97_sub_a(EMU *emu) {
    emu->cpu_.a = sub_8(emu, emu->cpu_.a, emu->cpu_.a);
    emu->tick(1);
}
void x98_sbc_a_b(EMU *emu) {
    emu->cpu_.a = sbc_8(emu, emu->cpu_.a, emu->cpu_.b);
    emu->tick(1);
}
void x99_sbc_a_c(EMU *emu) {
    emu->cpu_.a = sbc_8(emu, emu->cpu_.a, emu->cpu_.c);
    emu->tick(1);
}
void x9a_sbc_a_d(EMU *emu) {
    emu->cpu_.a = sbc_8(emu, emu->cpu_.a, emu->cpu_.d);
    emu->tick(1);
}
void x9b_sbc_a_e(EMU *emu) {
    emu->cpu_.a = sbc_8(emu, emu->cpu_.a, emu->cpu_.e);
    emu->tick(1);
}
void x9c_sbc_a_h(EMU *emu) {
    emu->cpu_.a = sbc_8(emu, emu->cpu_.a, emu->cpu_.h);
    emu->tick(1);
}
void x9d_sbc_a_l(EMU *emu) {
    emu->cpu_.a = sbc_8(emu, emu->cpu_.a, emu->cpu_.l);
    emu->tick(1);
}
void x9e_sbc_a_mhl(EMU *emu) {
    emu->cpu_.a = sbc_8(emu, emu->cpu_.a, emu->bus_read(emu->cpu_.hl()));
    emu->tick(2);
}
void x9f_sbc_a_a(EMU *emu) {
    emu->cpu_.a = sbc_8(emu, emu->cpu_.a, emu->cpu_.a);
    emu->tick(1);
}
void xa0_and_b(EMU *emu) {
    emu->cpu_.a = and_8(emu, emu->cpu_.a, emu->cpu_.b);
    emu->tick(1);
}
void xa1_and_c(EMU *emu) {
    emu->cpu_.a = and_8(emu, emu->cpu_.a, emu->cpu_.c);
    emu->tick(1);
}
void xa2_and_d(EMU *emu) {
    emu->cpu_.a = and_8(emu, emu->cpu_.a, emu->cpu_.d);
    emu->tick(1);
}
void xa3_and_e(EMU *emu) {
    emu->cpu_.a = and_8(emu, emu->cpu_.a, emu->cpu_.e);
    emu->tick(1);
}
void xa4_and_h(EMU *emu) {
    emu->cpu_.a = and_8(emu, emu->cpu_.a, emu->cpu_.h);
    emu->tick(1);
}
void xa5_and_l(EMU *emu) {
    emu->cpu_.a = and_8(emu, emu->cpu_.a, emu->cpu_.l);
    emu->tick(1);
}
void xa6_and_mhl(EMU *emu) {
    emu->cpu_.a = and_8(emu, emu->cpu_.a, emu->bus_read(emu->cpu_.hl()));
    emu->tick(2);
}
void xa7_and_a(EMU *emu) {
    emu->cpu_.a = and_8(emu, emu->cpu_.a, emu->cpu_.a);
    emu->tick(1);
}
void xa8_xor_b(EMU *emu) {
    emu->cpu_.a = xor_8(emu, emu->cpu_.a, emu->cpu_.b);
    emu->tick(1);
}
void xa9_xor_c(EMU *emu) {
    emu->cpu_.a = xor_8(emu, emu->cpu_.a, emu->cpu_.c);
    emu->tick(1);
}
void xaa_xor_d(EMU *emu) {
    emu->cpu_.a = xor_8(emu, emu->cpu_.a, emu->cpu_.d);
    emu->tick(1);
}
void xab_xor_e(EMU *emu) {
    emu->cpu_.a = xor_8(emu, emu->cpu_.a, emu->cpu_.e);
    emu->tick(1);
}
void xac_xor_h(EMU *emu) {
    emu->cpu_.a = xor_8(emu, emu->cpu_.a, emu->cpu_.h);
    emu->tick(1);
}
void xad_xor_l(EMU *emu) {
    emu->cpu_.a = xor_8(emu, emu->cpu_.a, emu->cpu_.l);
    emu->tick(1);
}
void xae_xor_mhl(EMU *emu) {
    emu->cpu_.a = xor_8(emu, emu->cpu_.a, emu->bus_read(emu->cpu_.hl()));
    emu->tick(2);
}
void xaf_xor_a(EMU *emu) {
    emu->cpu_.a = xor_8(emu, emu->cpu_.a, emu->cpu_.a);
    emu->tick(1);
}
void xb0_or_b(EMU *emu) {
    emu->cpu_.a = or_8(emu, emu->cpu_.a, emu->cpu_.b);
    emu->tick(1);
}
void xb1_or_c(EMU *emu) {
    emu->cpu_.a = or_8(emu, emu->cpu_.a, emu->cpu_.c);
    emu->tick(1);
}
void xb2_or_d(EMU *emu) {
    emu->cpu_.a = or_8(emu, emu->cpu_.a, emu->cpu_.d);
    emu->tick(1);
}
void xb3_or_e(EMU *emu) {
    emu->cpu_.a = or_8(emu, emu->cpu_.a, emu->cpu_.e);
    emu->tick(1);
}
void xb4_or_h(EMU *emu) {
    emu->cpu_.a = or_8(emu, emu->cpu_.a, emu->cpu_.h);
    emu->tick(1);
}
void xb5_or_l(EMU *emu) {
    emu->cpu_.a = or_8(emu, emu->cpu_.a, emu->cpu_.l);
    emu->tick(1);
}
void xb6_or_mhl(EMU *emu) {
    emu->cpu_.a = or_8(emu, emu->cpu_.a, emu->bus_read(emu->cpu_.hl()));
    emu->tick(2);
}
void xb7_or_a(EMU *emu) {
    emu->cpu_.a = or_8(emu, emu->cpu_.a, emu->cpu_.a);
    emu->tick(1);
}
void xb8_cp_b(EMU *emu) {
    cp_8(emu, emu->cpu_.a, emu->cpu_.b);
    emu->tick(1);
}
void xb9_cp_c(EMU *emu) {
    cp_8(emu, emu->cpu_.a, emu->cpu_.c);
    emu->tick(1);
}
void xba_cp_d(EMU *emu) {
    cp_8(emu, emu->cpu_.a, emu->cpu_.d);
    emu->tick(1);
}
void xbb_cp_e(EMU *emu) {
    cp_8(emu, emu->cpu_.a, emu->cpu_.e);
    emu->tick(1);
}
void xbc_cp_h(EMU *emu) {
    cp_8(emu, emu->cpu_.a, emu->cpu_.h);
    emu->tick(1);
}
void xbd_cp_l(EMU *emu) {
    cp_8(emu, emu->cpu_.a, emu->cpu_.l);
    emu->tick(1);
}
void xbe_cp_mhl(EMU *emu) {
    cp_8(emu, emu->cpu_.a, emu->bus_read(emu->cpu_.hl()));
    emu->tick(2);
}
void xbf_cp_a(EMU *emu) {
    cp_8(emu, emu->cpu_.a, emu->cpu_.a);
    emu->tick(1);
}
void xc0_ret_nz(EMU *emu) {
    if (!emu->cpu_.fz()) {
        emu->cpu_.pc = pop_16(emu);
        emu->tick(5);
    } else {
        emu->tick(2);
    }
}
void xc1_pop_bc(EMU *emu) {
    emu->cpu_.bc(pop_16(emu));
    emu->tick(3);
}
void xc2_jp_nz_a16(EMU *emu) {
    if (!emu->cpu_.fz()) {
        emu->cpu_.pc = read_d16(emu);
        emu->tick(4);
    } else {
        emu->tick(3);
    }
}
void xc3_jp_a16(EMU *emu) {
    emu->cpu_.pc = read_d16(emu);
    emu->tick(4);
}
void xc4_call_nz_a16(EMU *emu) {
    if (!emu->cpu_.fz()) {
        u16 address = read_d16(emu);
        push_16(emu, emu->cpu_.pc);
        emu->cpu_.pc = address;
        emu->tick(6);
    } else {
        emu->tick(3);
    }
}
void xc5_push_bc(EMU *emu) {
    push_16(emu, emu->cpu_.bc());
    emu->tick(4);
}
void xc6_add_a_d8(EMU *emu) {
    emu->cpu_.a = add_8(emu, emu->cpu_.a, read_d8(emu));
    emu->tick(2);
}
void xc7_rst_00h(EMU *emu) {
    push_16(emu, emu->cpu_.pc);
    emu->cpu_.pc = 0x0000;
    emu->tick(4);
}
void xc8_ret_z(EMU *emu) {
    if (emu->cpu_.fz()) {
        emu->cpu_.pc = pop_16(emu);
        emu->tick(5);
    } else {
        emu->tick(2);
    }
}
void xc9_ret(EMU *emu) {
    emu->cpu_.pc = pop_16(emu);
    emu->tick(4);
}
void xca_jp_z_a16(EMU *emu) {
    if (emu->cpu_.fz()) {
        emu->cpu_.pc = read_d16(emu);
        emu->tick(4);
    } else {
        emu->tick(3);
    }
}
void xcb_prefix_cb(EMU *emu) {
    u8 op = read_d8(emu);
    emu->tick(1);
    u8 data_bits = op & 0x07;
    u8 op_bits = (op & 0xF8) >> 3;
    u8 data;
    switch(data_bits) {
        case 0: data = emu->cpu_.b; break;
        case 1: data = emu->cpu_.c; break;
        case 2: data = emu->cpu_.d; break;
        case 3: data = emu->cpu_.e; break;
        case 4: data = emu->cpu_.h; break;
        case 5: data = emu->cpu_.l; break;
        case 6: data = emu->bus_read(emu->cpu_.hl()); emu->tick(1); break;
        case 7: data = emu->cpu_.a; break;
        default:
        std::cerr << "Invalid data bits in CB prefix: " << (int)data_bits << std::endl;
        break;
    }
    if(op_bits == 0) {
        // RLC
        rlc_8(emu, data);
    } else if(op_bits == 1) {
        // RRC
        rrc_8(emu, data);
    } else if(op_bits == 2) {
        // RL
        rl_8(emu, data);
    } else if(op_bits == 3) {
        // RR
        rr_8(emu, data);
    } else if(op_bits == 4) {
        // SLA
        sla_8(emu, data);
    } else if(op_bits == 5) {
        // SRA
        sra_8(emu, data);
    } else if(op_bits == 6) {
        // SWAP
        swap_8(emu, data);
    } else if(op_bits == 7) {
        // SRL
       srl_8(emu, data);
    }
    else if(op_bits <= 0x0F) {
        bit_8(emu, data, op_bits-0x08);
    }
    else if(op_bits <= 0x1F) {
        res_8(emu, data, op_bits-0x10);
    } else if(op_bits <= 0x2F) {
        set_8(emu, data, op_bits-0x18);
    } else {
        std::cerr << "Invalid CB prefix operation: " << (int)op_bits << std::endl;
    }
    switch (data_bits) {
        case 0: emu->cpu_.b = data; break;
        case 1: emu->cpu_.c = data; break;
        case 2: emu->cpu_.d = data; break;
        case 3: emu->cpu_.e = data; break;
        case 4: emu->cpu_.h = data; break;
        case 5: emu->cpu_.l = data; break;
        case 6: emu->bus_write(emu->cpu_.hl(), data); emu->tick(1); break;
        case 7: emu->cpu_.a = data; break;
        default:
        std::cerr << "Invalid data bits in CB prefix: " << (int)data_bits << std::endl;
        break;
    }
    emu->tick(1);
}
void xcc_call_z_a16(EMU *emu) {
    if (emu->cpu_.fz()) {
        u16 address = read_d16(emu);
        push_16(emu, emu->cpu_.pc);
        emu->cpu_.pc = address;
        emu->tick(6);
    } else {
        emu->tick(3);
    }
}
void xcd_call_a16(EMU *emu) {
    u16 address = read_d16(emu);
    push_16(emu, emu->cpu_.pc);
    emu->cpu_.pc = address;
    emu->tick(6);
}
void xce_adc_a_d8(EMU *emu) {
    emu->cpu_.a = adc_8(emu, emu->cpu_.a, read_d8(emu));
    emu->tick(2);
}
void xcf_rst_08h(EMU *emu) {
    push_16(emu, emu->cpu_.pc);
    emu->cpu_.pc = 0x0008;
    emu->tick(4);
}
void xd0_ret_nc(EMU *emu) {
    if (!emu->cpu_.fc()) {
        emu->cpu_.pc = pop_16(emu);
        emu->tick(5);
    } else {
        emu->tick(2);
    }
}
void xd1_pop_de(EMU *emu) {
    emu->cpu_.de(pop_16(emu));
    emu->tick(3);
}
void xd2_jp_nc_a16(EMU *emu) {
    if (!emu->cpu_.fc()) {
        emu->cpu_.pc = read_d16(emu);
        emu->tick(4);
    } else {
        emu->tick(3);
    }
}
void xd4_call_nc_a16(EMU *emu) {
    if (!emu->cpu_.fc()) {
        u16 address = read_d16(emu);
        push_16(emu, emu->cpu_.pc);
        emu->cpu_.pc = address;
        emu->tick(6);
    } else {
        emu->tick(3);
    }
}
void xd5_push_de(EMU *emu) {
    push_16(emu, emu->cpu_.de());
    emu->tick(4);
}
void xd6_sub_d8(EMU *emu) {
    emu->cpu_.a = sub_8(emu, emu->cpu_.a, read_d8(emu));
    emu->tick(2);
}
void xd7_rst_10h(EMU *emu) {
    push_16(emu, emu->cpu_.pc);
    emu->cpu_.pc = 0x0010;
    emu->tick(4);
}
void xd8_ret_c(EMU *emu) {
    if (emu->cpu_.fc()) {
        emu->cpu_.pc = pop_16(emu);
        emu->tick(5);
    } else {
        emu->tick(2);
    }
}
void xd9_reti(EMU *emu) {
  emu->cpu_.enable_interrupt_master();
  xc9_ret(emu);
}
void xda_jp_c_a16(EMU *emu) {
    if (emu->cpu_.fc()) {
        emu->cpu_.pc = read_d16(emu);
        emu->tick(4);
    } else {
        emu->tick(3);
    }
}
void xdc_call_c_a16(EMU *emu) {
    if (emu->cpu_.fc()) {
        u16 address = read_d16(emu);
        push_16(emu, emu->cpu_.pc);
        emu->cpu_.pc = address;
        emu->tick(6);
    } else {
        emu->tick(3);
    }
}
void xde_sbc_a_d8(EMU *emu) {
    emu->cpu_.a = sbc_8(emu, emu->cpu_.a, read_d8(emu));
    emu->tick(2);
}
void xdf_rst_18h(EMU *emu) {
    push_16(emu, emu->cpu_.pc);
    emu->cpu_.pc = 0x0018;
    emu->tick(4);
}
void xe0_ldh_m8_a(EMU *emu) {
    emu->bus_write(0xFF00 + static_cast<u16>(read_d8(emu)), emu->cpu_.a);
    emu->tick(3);
}
void xe1_pop_hl(EMU *emu) {
    emu->cpu_.hl(pop_16(emu));
    emu->tick(3);
}
void xe2_ld_mc_a(EMU *emu) {
    emu->bus_write(0xFF00 + static_cast<u16>(emu->cpu_.c), emu->cpu_.a);
    emu->tick(2);
}
void xe5_push_hl(EMU *emu) {
    push_16(emu, emu->cpu_.hl());
    emu->tick(4);
}
void xe6_and_d8(EMU *emu) {
    emu->cpu_.a = and_8(emu, emu->cpu_.a, read_d8(emu));
    emu->tick(2);
}
void xe7_rst_20h(EMU *emu) {
    push_16(emu, emu->cpu_.pc);
    emu->cpu_.pc = 0x0020;
    emu->tick(4);
}
void xe8_add_sp_r8(EMU *emu) {
    emu->cpu_.reset_fz();
    emu->cpu_.reset_fn();
    u16 v1 = emu->cpu_.sp;
    s16 v2 = static_cast<s16>(static_cast<s8>(read_d8(emu)));
    u16 temp = v1 + v2;
    emu->cpu_.sp = temp;
    u16 check = v1 ^ v2 ^ temp;
    if (check & 0x100) {
        emu->cpu_.set_fc();
    } else {
        emu->cpu_.reset_fc();
    }
    if (check & 0x10) {
        emu->cpu_.set_fh();
    } else {
        emu->cpu_.reset_fh();
    }
    emu->tick(3);
}
void xe9_jp_hl(EMU *emu) {
    emu->cpu_.pc = emu->cpu_.hl();
    emu->tick(1);
}
void xea_ld_a16_a(EMU *emu) {
    u16 address = read_d16(emu);
    emu->bus_write(address, emu->cpu_.a);
    emu->tick(4);
}
void xee_xor_d8(EMU *emu) {
    emu->cpu_.a = xor_8(emu, emu->cpu_.a, read_d8(emu));
    emu->tick(2);
}
void xef_rst_28h(EMU *emu) {
    push_16(emu, emu->cpu_.pc);
    emu->cpu_.pc = 0x0028;
    emu->tick(4);
}
void xf0_ldh_a_m8(EMU *emu) {
    emu->cpu_.a = emu->bus_read(0xFF00 + static_cast<u16>(read_d8(emu)));
    emu->tick(3);
}
void xf1_pop_af(EMU *emu) {
    emu->cpu_.af(pop_16(emu));
    emu->tick(3);
}
void xf2_ld_a_mc(EMU *emu) {
    emu->cpu_.a = emu->bus_read(0xFF00 + static_cast<u16>(emu->cpu_.c));
    emu->tick(2);
}
void xf3_di(EMU *emu) {
    emu->cpu_.disable_interrupt_master();
    emu->tick(1);
}
void xf5_push_af(EMU *emu) {
    push_16(emu, emu->cpu_.af());
    emu->tick(4);
}
void xf6_or_d8(EMU *emu) {
    emu->cpu_.a = or_8(emu, emu->cpu_.a, read_d8(emu));
    emu->tick(2);
}
void xf7_rst_30h(EMU *emu) {
    push_16(emu, emu->cpu_.pc);
    emu->cpu_.pc = 0x0030;
    emu->tick(4);
}
void xf8_ld_hl_sp_r8(EMU *emu) {
  emu->cpu_.reset_fz();
  emu->cpu_.reset_fn();
  u16 v1 = emu->cpu_.sp;
  s16 v2 = static_cast<s16>(static_cast<s8>(read_d8(emu)));
  u16 temp = v1 + v2;
  emu->cpu_.hl(temp);
  u16 check = v1 & v2 & temp;
  if (check & 0x100) {
    emu->cpu_.set_fc();
  } else {
    emu->cpu_.reset_fc();
  }
  if (check & 0x10) {
    emu->cpu_.set_fh();
  } else {
    emu->cpu_.reset_fh();
  }
  emu->tick(3);
}
void xf9_ld_sp_hl(EMU *emu) {
    emu->cpu_.sp = emu->cpu_.hl();
    emu->tick(2);
}
void xfa_ld_a_a16(EMU *emu) {
    u16 address = read_d16(emu);
    emu->cpu_.a = emu->bus_read(address);
    emu->tick(4);
}
void xfb_ei(EMU *emu) {
    emu->cpu_.enable_interrupt_master();
    emu->tick(1);
}
void xfe_cp_d8(EMU *emu) {
    cp_8(emu, emu->cpu_.a, read_d8(emu));
    emu->tick(2);
}
void xff_rst_38h(EMU *emu) {
    push_16(emu, emu->cpu_.pc);
    emu->cpu_.pc = 0x0038;
    emu->tick(4);
}
instruction_func_t instructions_map[256] = {
    x00_nop     , x01_ld_bc_d16, x02_ld_mbc_a , x03_inc_bc  , x04_inc_b      , x05_dec_b   , x06_ld_b_d8  , x07_rlca    , x08_ld_a16_sp  , x09_add_hl_bc, x0a_ld_a_mbc , x0b_dec_bc   , x0c_inc_c     , x0d_dec_c   , x0e_ld_c_d8  , x0f_rrca, 
    x10_stop    , x11_ld_de_d16, x12_ld_mde_a , x13_inc_de  , x14_inc_d      , x15_dec_d   , x16_ld_d_d8  , x17_rla     , x18_jr_r8      , x19_add_hl_de, x1a_ld_a_mde , x1b_dec_de   , x1c_inc_e     , x1d_dec_e   , x1e_ld_e_d8  , x1f_rra, 
    x20_jr_nz_r8, x21_ld_hl_d16, x22_ldi_mhl_a, x23_inc_hl  , x24_inc_h      , x25_dec_h   , x26_ld_h_d8  , x27_daa     , x28_jr_z_r8    , x29_add_hl_hl, x2a_ldi_a_mhl, x2b_dec_hl   , x2c_inc_l     , x2d_dec_l   , x2e_ld_l_d8  , x2f_cpl, 
    x30_jr_nc_r8, x31_ld_sp_d16, x32_ldd_mhl_a, x33_inc_sp  , x34_inc_mhl    , x35_dec_mhl , x36_ld_mhl_d8, x37_scf     , x38_jr_c_r8    , x39_add_hl_sp, x3a_ldd_a_mhl, x3b_dec_sp   , x3c_inc_a     , x3d_dec_a   , x3e_ld_a_d8  , x3f_ccf, 
    x40_ld_b_b  , x41_ld_b_c   , x42_ld_b_d   , x43_ld_b_e  , x44_ld_b_h     , x45_ld_b_l  , x46_ld_b_mhl , x47_ld_b_a  , x48_ld_c_b     , x49_ld_c_c   , x4a_ld_c_d   , x4b_ld_c_e   , x4c_ld_c_h    , x4d_ld_c_l  , x4e_ld_c_mhl , x4f_ld_c_a,
    x50_ld_d_b  , x51_ld_d_c   , x52_ld_d_d   , x53_ld_d_e  , x54_ld_d_h     , x55_ld_d_l  , x56_ld_d_mhl , x57_ld_d_a  , x58_ld_e_b     , x59_ld_e_c   , x5a_ld_e_d   , x5b_ld_e_e   , x5c_ld_e_h    , x5d_ld_e_l  , x5e_ld_e_mhl , x5f_ld_e_a,
    x60_ld_h_b  , x61_ld_h_c   , x62_ld_h_d   , x63_ld_h_e  , x64_ld_h_h     , x65_ld_h_l  , x66_ld_h_mhl , x67_ld_h_a  , x68_ld_l_b     , x69_ld_l_c   , x6a_ld_l_d   , x6b_ld_l_e   , x6c_ld_l_h    , x6d_ld_l_l  , x6e_ld_l_mhl , x6f_ld_l_a,
    x70_ld_mhl_b, x71_ld_mhl_c , x72_ld_mhl_d , x73_ld_mhl_e, x74_ld_mhl_h   , x75_ld_mhl_l, x76_halt     , x77_ld_mhl_a, x78_ld_a_b     , x79_ld_a_c   , x7a_ld_a_d   , x7b_ld_a_e   , x7c_ld_a_h    , x7d_ld_a_l  , x7e_ld_a_mhl , x7f_ld_a_a,
    x80_add_a_b , x81_add_a_c  , x82_add_a_d  , x83_add_a_e , x84_add_a_h    , x85_add_a_l , x86_add_a_mhl, x87_add_a_a , x88_adc_a_b    , x89_adc_a_c  , x8a_adc_a_d  , x8b_adc_a_e  , x8c_adc_a_h   , x8d_adc_a_l , x8e_adc_a_mhl, x8f_adc_a_a,
    x90_sub_b   , x91_sub_c    , x92_sub_d    , x93_sub_e   , x94_sub_h      , x95_sub_l   , x96_sub_mhl  , x97_sub_a   , x98_sbc_a_b    , x99_sbc_a_c  , x9a_sbc_a_d  , x9b_sbc_a_e  , x9c_sbc_a_h   , x9d_sbc_a_l , x9e_sbc_a_mhl, x9f_sbc_a_a,
    xa0_and_b   , xa1_and_c    , xa2_and_d    , xa3_and_e   , xa4_and_h      , xa5_and_l   , xa6_and_mhl  , xa7_and_a   , xa8_xor_b      , xa9_xor_c    , xaa_xor_d    , xab_xor_e    , xac_xor_h     , xad_xor_l   , xae_xor_mhl  , xaf_xor_a,
    xb0_or_b    , xb1_or_c     , xb2_or_d     , xb3_or_e    , xb4_or_h       , xb5_or_l    , xb6_or_mhl   , xb7_or_a    , xb8_cp_b       , xb9_cp_c     , xba_cp_d     , xbb_cp_e     , xbc_cp_h      , xbd_cp_l    , xbe_cp_mhl   , xbf_cp_a,
    xc0_ret_nz  , xc1_pop_bc   , xc2_jp_nz_a16, xc3_jp_a16  , xc4_call_nz_a16, xc5_push_bc , xc6_add_a_d8 , xc7_rst_00h , xc8_ret_z      , xc9_ret      , xca_jp_z_a16 , xcb_prefix_cb, xcc_call_z_a16, xcd_call_a16, xce_adc_a_d8 , xcf_rst_08h,
    xd0_ret_nc  , xd1_pop_de   , xd2_jp_nc_a16, nullptr     , xd4_call_nc_a16, xd5_push_de , xd6_sub_d8   , xd7_rst_10h , xd8_ret_c      , xd9_reti     , xda_jp_c_a16 , nullptr      , xdc_call_c_a16, nullptr     , xde_sbc_a_d8 , xdf_rst_18h,
    xe0_ldh_m8_a, xe1_pop_hl   , xe2_ld_mc_a  , nullptr     , nullptr        , xe5_push_hl , xe6_and_d8   , xe7_rst_20h , xe8_add_sp_r8  , xe9_jp_hl    , xea_ld_a16_a , nullptr      , nullptr       , nullptr     , xee_xor_d8   , xef_rst_28h, 
    xf0_ldh_a_m8, xf1_pop_af   , xf2_ld_a_mc  , xf3_di      , nullptr        , xf5_push_af , xf6_or_d8    , xf7_rst_30h , xf8_ld_hl_sp_r8, xf9_ld_sp_hl , xfa_ld_a_a16 , xfb_ei       , nullptr       , nullptr     , xfe_cp_d8    , xff_rst_38h};