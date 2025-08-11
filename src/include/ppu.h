#ifndef __PPU_H__
#define __PPU_H__
#include"defs.h"
class EMU;

enum class PPUMode : u8
{
    hblank = 0,
    vblank = 1,
    oam_scan = 2,
    drawing = 3
};
constexpr u32 PPU_LINES_PER_FRAME = 154;
constexpr u32 PPU_CYCLES_PER_LINE = 456;
constexpr u32 PPU_YRES = 144;
constexpr u32 PPU_XRES = 160;
class PPU {
    public:

    void init();
    void tick(EMU *emu);
    u8 bus_read(u16 addr);
    void bus_write(u16 addr, u8 data);
    void tick_oam_scan(EMU *emu);
    void tick_drawing(EMU *emu);
    void tick_hblank(EMU *emu);
    void tick_vblank(EMU *emu);

    
    void increase_ly(EMU *emu);
    bool enabled()const{return lcdc & 0x80;}
    PPUMode get_mode()const{return static_cast<PPUMode>(lcds & 0x03);}
    void set_mode(PPUMode mode){lcds = (lcds & 0xFC) | static_cast<u8>(mode);}
    void set_lyc_flag(){lcds |= 0x04;}//0x04 = 0000 0100
    void reset_lyc_flag(){lcds &= 0xFB;}//0xFB = 1111 1011
    bool hblank_int_enabled() const { return !!(lcds & (1 << 3)); }
    bool vblank_int_enabled() const { return !!(lcds & (1 << 4)); }
    bool oam_int_enabled() const { return !!(lcds & (1 << 5)); }
    bool lyc_int_enabled() const { return !!(lcds & (1 << 6)); }

    u32 line_cycles;
    u8 ppu_reg_[0x0C];
    //! 0xFF40 - LCD control.
    u8 &lcdc = ppu_reg_[0];
    //! 0xFF41 - LCD status.
    u8 &lcds = ppu_reg_[1];
    //! 0xFF42 - SCY.
    u8 &scroll_y = ppu_reg_[2];
    //! 0xFF43 - SCX.
    u8 &scroll_x = ppu_reg_[3];
    //! 0xFF44 - LY LCD Y coordinate [Read Only].
    u8 &ly = ppu_reg_[4];
    //! 0xFF45 - LYC LCD Y Compare.
    u8 &lyc = ppu_reg_[5];
    //! 0xFF46 - DMA value.
    u8 &dma = ppu_reg_[6];
    //! 0xFF47 - BGP (BG palette data).
    u8 &bgp = ppu_reg_[7];
    //! 0xFF48 - OBP0 (OBJ0 palette data).
    u8 &obp0 = ppu_reg_[8];
    //! 0xFF49 - OBP1 (OBJ1 palette data).
    u8 &obp1 = ppu_reg_[9];
    //! 0xFF4A - WY (Window Y position plus 7).
    u8 &wy = ppu_reg_[10];
    //! 0xFF4B - WX (Window X position plus 7).
    u8 &wx = ppu_reg_[11];
};

#endif // __PPU_H__