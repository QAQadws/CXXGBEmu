#ifndef __PPU_H__
#define __PPU_H__
#include"defs.h"
#include<deque>

constexpr u32 PPU_LINES_PER_FRAME = 154;
constexpr u32 PPU_CYCLES_PER_LINE = 456;
constexpr u32 PPU_YRES = 144;
constexpr u32 PPU_XRES = 160;
class EMU;
enum class PPUMode : u8
{
    hblank = 0,
    vblank = 1,
    oam_scan = 2,
    drawing = 3
};


enum class PPUFetchState : u8 { tile, data0, data1, idle, push };
struct BGWPixel {
  //! The color index.
  u8 color;
  //! The palette used for this pixel.
  u8 palette;
};

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
    void fetcher_get_tile(EMU *emu);
    void fetcher_get_data(EMU *emu, u8 data_index);
    void fetcher_push_pixels();
    void lcd_draw_pixel();
    void fetcher_get_window_tile(EMU *emu);
    void fetcher_get_background_tile(EMU *emu);

    u8 pixels[PPU_XRES * PPU_YRES * 4 * 2]{};//TODO double buffer
    u8 current_back_buffer{};
    void set_pixel(s32 x, s32 y, u8 r, u8 g, u8 b, u8 a);
    void fetcher_push_bgw_pixels();

    bool bg_window_enabled() const { return (lcdc & 0x01); }
    bool window_enabled()const { return (lcdc & (0x01 << 5)); }
    u16 bg_map_area()const {return (lcdc & (0x01 << 3)) ? 0x9C00 : 0x9800;}
    u16 window_map_area()const {return (lcdc & (0x01<<6)) ? 0x9C00 : 0x9800;}
    u16 bgw_data_area()const {return (lcdc & (0x01 << 4)) ? 0x8000 : 0x8800;}
    bool window_visible()const { return window_enabled() && wx <=166 && wy < PPU_YRES; }
    bool is_pixel_window(u8 screen_x, u8 screen_y) const {return window_visible() && (screen_x + 7 >= wx) && (screen_y >= wy);}

    bool enabled()const{return lcdc & 0x80;}
    PPUMode get_mode()const{return static_cast<PPUMode>(lcds & 0x03);}
    void set_mode(PPUMode mode){lcds = (lcds & 0xFC) | static_cast<u8>(mode);}
    void set_lyc_flag(){lcds |= 0x04;}//0x04 = 0000 0100
    void reset_lyc_flag(){lcds &= 0xFB;}//0xFB = 1111 1011
    bool hblank_int_enabled() const { return !!(lcds & (1 << 3)); }
    bool vblank_int_enabled() const { return !!(lcds & (1 << 4)); }
    bool oam_int_enabled() const { return !!(lcds & (1 << 5)); }
    bool lyc_int_enabled() const { return !!(lcds & (1 << 6)); }

    std::deque<BGWPixel> bgw_queue{};
    bool fetch_window{};
    u8 window_line{};
    PPUFetchState fetch_state{};
    u8 fetch_x{};
    u16 bgw_data_addr_offset{};
    s16 tile_x_begin{};
    u8 bgw_fetched_data[2]{};
    u8 push_x{};
    u8 draw_x{};

    u32 line_cycles{};
    u8 ppu_reg_[0x0C]{};
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