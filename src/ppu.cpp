#include"ppu.h"
#include <iostream>
#include"emu.h"

inline u8 apply_palette(u8 color, u8 palette) {
  switch (color) {
  case 0:
    color = palette & 0x03;
    break;
  case 1:
    color = ((palette >> 2) & 0x03);
    break;
  case 2:
    color = ((palette >> 4) & 0x03);
    break;
  case 3:
    color = ((palette >> 6) & 0x03);
    break;
  default:
  std::cerr << "Invalid color index" << std::endl;
    break;
  }
  return color;
}
void PPU::init()
{
  lcdc = 0x91;//0x91 = 1001 0001
  lcds = 0;
  scroll_y = 0;
  scroll_x = 0;
  ly = 0;
  lyc = 0;
  dma = 0;
  bgp = 0xFC;
  obp0 = 0xFF;
  obp1 = 0xFF;
  wy = 0;
  wx = 0;
  set_mode(PPUMode::oam_scan);

  line_cycles = 0;
  fetch_state = PPUFetchState::tile;
  fetch_window = false;
  fetch_x = 0;
  push_x = 0;
  draw_x = 0;
  window_line = 0;
  current_back_buffer = 0;
}

void PPU::tick(EMU *emu)
{
    if(!enabled()) {return;}
    ++line_cycles;
    switch(get_mode()){
    case PPUMode::oam_scan:
      tick_oam_scan(emu);
      break;
    case PPUMode::drawing:
      tick_drawing(emu);
      break;
    case PPUMode::hblank:
      tick_hblank(emu);
      break;
    case PPUMode::vblank:
      tick_vblank(emu);
      break;
    }
}

u8 PPU::bus_read(u16 addr)
{
    if(addr>=0xFF40 && addr<=0xFF4B){
        return ppu_reg_[addr - 0xFF40];
    }
    std::cerr << "PPU: Invalid read from " << std::hex << addr << std::endl;
    return 0xFF;
}

void PPU::bus_write(u16 addr, u8 data)
{
    if(addr>=0xFF40 && addr<=0xFF4B){
      if (addr == 0xFF40 && enabled() && !(data & 0x80)) {
        // Reset mode to HBLANK.
        lcds &= 0x7C;
        // Reset LY.
        ly = 0;
        line_cycles = 0;
      }
      if (addr == 0xFF41) // the lower 3 bits are read only.
      {
        lcds = (lcds & 0x07) | (data & 0xF8);
        return;
      }
      if (addr == 0xFF44){return;} // read only.
      ppu_reg_[addr - 0xFF40] = data;
    }
}

void PPU::tick_oam_scan(EMU *emu)
{
  if (line_cycles >= 80) {
    set_mode(PPUMode::drawing);
    fetch_window = false;
    fetch_x = scroll_x & 0xF8;
    push_x = 0;
    draw_x = 0;
    bgw_queue.clear();
    fetch_state = PPUFetchState::tile;
  }
}

void PPU::tick_drawing(EMU *emu)
{
  if ((line_cycles % 2) == 0) {
    switch(fetch_state){
        case PPUFetchState::tile:
            fetcher_get_tile(emu);break;
        case PPUFetchState::data0:
            fetcher_get_data(emu, 0);break;
        case PPUFetchState::data1:
            fetcher_get_data(emu, 1);break;
        case PPUFetchState::idle:
            fetch_state = PPUFetchState::push;break;
        case PPUFetchState::push:
            fetcher_push_pixels();break;
    }
    if (draw_x >= PPU_XRES) {
      if (line_cycles >= 252 && line_cycles <= 369) {
        set_mode(PPUMode::hblank);
        if (hblank_int_enabled()) {
          emu->int_flags |= INT_LCD_STAT;
        }
      }
      else{
        std::cerr << "PPU: Invalid line_cycles " << line_cycles << " at draw_x " << draw_x << std::endl;
      }
    }
  }
  lcd_draw_pixel();
}

void PPU::tick_hblank(EMU *emu)
{
    if(line_cycles >= PPU_CYCLES_PER_LINE){
        increase_ly(emu);
        if(ly >= PPU_YRES){
            set_mode(PPUMode::vblank);
            emu->int_flags |= INT_VBLANK;
            if(vblank_int_enabled()){
                emu->int_flags |= INT_LCD_STAT;
            }
            current_back_buffer = (current_back_buffer + 1) % 2; // Switch back buffer
        }
        else{
            set_mode(PPUMode::oam_scan);
            if(oam_int_enabled()){
                emu->int_flags |= INT_LCD_STAT;
            }
        }
        line_cycles = 0;
    }
}

void PPU::tick_vblank(EMU *emu)
{
    if(line_cycles >= PPU_CYCLES_PER_LINE){
        increase_ly(emu);
        if(ly >= PPU_LINES_PER_FRAME){
            set_mode(PPUMode::oam_scan);
            ly = 0;
            window_line = 0;
            if(oam_int_enabled()){
                emu->int_flags |= INT_LCD_STAT;
            }
        }
        line_cycles = 0;
    }
}

void PPU::increase_ly(EMU *emu)
{
    if(window_visible() && ly >= wy && (u16)ly < (u16)(wy + PPU_YRES))
    {
        ++window_line;
    }
    ++ly;
    if(ly == lyc){
        set_lyc_flag();
        if(lyc_int_enabled()){
            emu->int_flags |= INT_LCD_STAT;
        }
    }
    else{
        reset_lyc_flag();
    }
}

void PPU::fetcher_get_tile(EMU *emu)
{
  if (bg_window_enabled()) {
    if (fetch_window) {
      fetcher_get_window_tile(emu);
    } else {
      fetcher_get_background_tile(emu);
    }
  }
  fetch_state = PPUFetchState::data0;
  fetch_x += 8;
}

void PPU::fetcher_get_data(EMU *emu, u8 data_index)
{
  if(bg_window_enabled()) {
    u16 addr = bgw_data_area() + bgw_data_addr_offset + data_index;
    u8 data = emu->bus_read(addr);
    bgw_fetched_data[data_index] = data;
  }
  if(data_index == 0){
    fetch_state = PPUFetchState::data1;
  }
  else{
    fetch_state = PPUFetchState::idle;
  }
}

void PPU::fetcher_push_pixels()
{
  bool pushed = false;
  if (bgw_queue.size() < 8) {
    fetcher_push_bgw_pixels();
    pushed = true;
  }
  if (pushed) {
    fetch_state = PPUFetchState::tile;
  }
}

void PPU::lcd_draw_pixel()
{
  if (bgw_queue.size() >= 8) {
    if (draw_x >= PPU_XRES)
      return;
    BGWPixel bgw_pixel = bgw_queue.front();
    bgw_queue.pop_front();
    // Calculate background color.
    u8 bg_color = apply_palette(bgw_pixel.color, bgw_pixel.palette);
    // Output pixel.
    switch (bg_color) {
    case 0:
      set_pixel(draw_x, ly, 153, 161, 120, 255);//不同的绿色
      break;
    case 1:
      set_pixel(draw_x, ly, 87, 93, 67, 255);
      break;
    case 2:
      set_pixel(draw_x, ly, 42, 46, 32, 255);
      break;
    case 3:
      set_pixel(draw_x, ly, 10, 10, 2, 255);
      break;
    }
    ++draw_x;
  }
}

void PPU::fetcher_get_window_tile(EMU *emu)
{
      u8 window_x = (fetch_x + 7 - wx);
    u8 window_y = window_line;
    u16 window_addr = window_map_area() + (window_x / 8) + ((window_y / 8) * 32);
    u8 tile_index = emu->bus_read(window_addr);
    if(bgw_data_area() == 0x8800)
    {

        tile_index += 128;
    }

    bgw_data_addr_offset = ((u16)tile_index * 16) + (u16)(window_y % 8) * 2;
    s32 tile_x = (s32)(fetch_x) - ((s32)(wx) - 7);
    tile_x = (tile_x / 8) * 8 + (s32)(wx) - 7;
    tile_x_begin = (s16)tile_x;
}
void PPU::fetcher_get_background_tile(EMU *emu)
{
    u8 map_y = ly + scroll_y;
    u8 map_x = fetch_x + scroll_x;
    u16 addr = bg_map_area() + (map_x / 8) + ((map_y / 8) * 32);
    // Read tile index.
    u8 tile_index = emu->bus_read(addr);
    if(bgw_data_area() == 0x8800)
    {
        tile_index += 128;
    }
    bgw_data_addr_offset = ((u16)tile_index * 16) + (u16)(map_y % 8) * 2;
    s32 tile_x = fetch_x - (scroll_x % 8);
    tile_x_begin = (s16)tile_x;
}

void PPU::set_pixel(s32 x, s32 y, u8 r, u8 g, u8 b, u8 a)
{
  if (x < 0 || x >= PPU_XRES || y < 0 || y >= PPU_YRES) {
    std::cerr << "PPU: Attempt to set pixel out of bounds at (" << x << ", " << y << ")" << std::endl;
      return; // Out of bounds
  }
  u8* dst = pixels +current_back_buffer * PPU_XRES * PPU_YRES * 4;
  u8* pixel = dst + (y * PPU_XRES + x) * 4;
  pixel[0] = r; // Red
  pixel[1] = g; // Green
  pixel[2] = b; // Blue
  pixel[3] = a; // Alpha
}
void PPU::fetcher_push_bgw_pixels() {
  // Load tile data.
  u8 b1 = bgw_fetched_data[0];
  u8 b2 = bgw_fetched_data[1];

  // ✅ 计算需要跳过的像素数（由于滚动造成的）
  u8 pixels_to_skip = 0;
  if (!fetch_window && fetch_x == (scroll_x & 0xF8)) {
    pixels_to_skip = scroll_x & 0x07; // 跳过scroll_x的低3位指定的像素
  }

  // Process every pixel in this tile.
  for (u32 i = 0; i < 8; ++i) {
    // ✅ 跳过由于滚动需要跳过的像素
    if (i < pixels_to_skip) {
      continue;
    }

    // Skip pixels not in the screen.
    if (tile_x_begin + (s32)i < 0) {
      continue;
    }

    // If this is a window pixel, we reset fetcher to fetch window and discard
    // remaining pixels.
    if (!fetch_window && is_pixel_window(push_x, ly)) {
      fetch_window = true;
      fetch_x = push_x;
      break;
    }

    // Now we can stream pixel.
    BGWPixel pixel;
    if (bg_window_enabled()) {
      u8 b = 7 - i;
      u8 lo = (!!(b1 & (1 << b)));
      u8 hi = (!!(b2 & (1 << b))) << 1;
      pixel.color = hi | lo;
      pixel.palette = bgp;
    } else {
      pixel.color = 0;
      pixel.palette = 0;
    }
    bgw_queue.push_back(pixel);
    ++push_x;
  }
}