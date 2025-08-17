#include"ppu.h"
#include <iostream>
#include"emu.h"
constexpr u32 PPU_LINES_PER_FRAME = 154;
constexpr u32 PPU_CYCLES_PER_LINE = 456;
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

  dma_active = false;
  dma_offset = 0;
  dma_start_delay = 0;

  window_line = 0;
  current_back_buffer = 0;
}

void PPU::tick(EMU *emu)
{
    if((emu->clock_cycles_%4) == 0){
      tick_dma(emu);
    }
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
      if(addr == 0xFF46){
        dma_active = true;
        dma_offset = 0;
        dma_start_delay = 1;
      }
      ppu_reg_[addr - 0xFF40] = data;
    }
}

void PPU::tick_oam_scan(EMU *emu)
{
  if (line_cycles >= 80) {
    set_mode(PPUMode::drawing);
    fetch_window = false;
    fetch_x = 0;
    push_x = 0;
    draw_x = 0;
    bgw_queue.clear();
    fetch_state = PPUFetchState::tile;
  }
  if (line_cycles == 1) {
    sprites.clear();
    sprites.reserve(10);
    u8 sprite_height = obj_height();
    for (u8 i = 0; i < 40; ++i) {
      if (sprites.size() >= 10) {
        break;
      }
      OAMEntry *entry = (OAMEntry *)(emu->oam) + i;
      if (entry->y <= ly + 16 && entry->y + sprite_height > ly + 16) {
        auto iter = sprites.begin();
        while (iter != sprites.end()) {
          if (iter->x > entry->x)
            break;
          ++iter;
        }
        sprites.insert(iter, *entry);
      }
    }
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
        bgw_queue.clear();
        obj_queue.clear();
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
            // ✅ 移除这里的缓冲区切换，改为在VBlank结束时切换
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
            
            // ✅ 在VBlank结束时切换缓冲区，确保完整帧已渲染
            current_back_buffer = (current_back_buffer + 1) % 2;
            
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
  else{
    tile_x_begin = fetch_x;
  }
  if(obj_enabled()){
    fetcher_get_sprite_tile(emu);
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
  if(obj_enabled()){
    fetcher_get_sprite_data(emu,data_index);
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
    u8 push_begin = push_x;
    fetcher_push_bgw_pixels();
    u8 push_end = push_x;
    fetcher_push_sprite_pixels(push_begin, push_end);
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
    ObjectPixel obj_pixel = obj_queue.front();
    obj_queue.pop_front();
    // Calculate background color.
    u8 bg_color = apply_palette(bgw_pixel.color, bgw_pixel.palette);
    bool draw_obj =
        obj_pixel.color && (!obj_pixel.bg_priority || bg_color == 0);
    // Calculate obj color.
    u8 obj_color = apply_palette(obj_pixel.color, obj_pixel.palette & 0xFC);
    // Selects the final color.
    u8 color = draw_obj ? obj_color : bg_color;
    // Output pixel.
    switch (color) {
    case 0:
      set_pixel(draw_x, ly, 224, 248, 208, 255);//TODO不同的绿色
      break;
    case 1:
      set_pixel(draw_x, ly, 136, 192, 112, 255);
      break;
    case 2:
      set_pixel(draw_x, ly, 52, 104, 86, 255);
      break;
    case 3:
      set_pixel(draw_x, ly, 8, 24, 32, 255);
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
    s32 tile_x = (s32)(fetch_x) + (s32)(scroll_x);
    tile_x = (tile_x / 8) * 8 - (s32)scroll_x;
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
  // Process every pixel in this tile.
  for (u32 i = 0; i < 8; ++i) {
    // Skip pixels not in the screen.
    // Pushing pixels when tile_x_begin + i >= PPU_XRES is ok and
    // they will not be drawn by the LCD driver, and all undrawn pixels
    // will be discarded when HBLANK mode is entered.
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

void PPU::tick_dma(EMU *emu)
{
  if (!dma_active) {
    return;
  }
  if (dma_start_delay > 0) {
    --dma_start_delay;
    return;
  }
  emu->oam[dma_offset] = emu->bus_read((((u16)dma) * 0x100) + dma_offset);
  ++dma_offset;
  dma_active = dma_offset < 0xA0;
}

void PPU::fetcher_get_sprite_tile(EMU* emu){
  num_fetched_sprites = 0;
  for (u8 i = 0; i < (u8)sprites.size(); ++i) {
    s32 sp_x = (s32)sprites[i].x - 8;
    if (((sp_x >= tile_x_begin) && (sp_x < (tile_x_begin + 8))) ||
        ((sp_x + 7 >= tile_x_begin) && (sp_x + 7 < (tile_x_begin + 8)))) {
      fetched_sprites[num_fetched_sprites] = sprites[i];
      ++num_fetched_sprites;
    }
    if (num_fetched_sprites >= 3) {
      break;
    }
  }
}

void PPU::fetcher_get_sprite_data(EMU *emu, u8 data_index) {
  u8 sprite_height = obj_height();
  for (u8 i = 0; i < num_fetched_sprites; ++i) {
    u8 ty = (u8)(ly + 16 - fetched_sprites[i].y);
    if (fetched_sprites[i].y_flip()) {
      // Flip y in tile.
      ty = (sprite_height - 1) - ty;
    }
    u8 tile = fetched_sprites[i].tile;
    if (sprite_height == 16) {
      tile &= 0xFE; // Clear the last 1 bit if in double tile mode.
    }
    sprite_fetched_data[(i * 2) + data_index] =
        emu->bus_read(0x8000 + (tile * 16) + ty * 2 + data_index);
  }
}

void PPU::fetcher_push_sprite_pixels(u8 push_begin, u8 push_end) {
  for (u32 i = push_begin; i < push_end; ++i) {
    ObjectPixel pixel;
    // The default value is one transparent color.
    pixel.color = 0;
    pixel.palette = 0;
    pixel.bg_priority = true;
    if (obj_enabled()) {
      for (u8 s = 0; s < num_fetched_sprites; ++s) {
        s32 spx = (s32)fetched_sprites[s].x - 8;
        s32 offset = (s32)(i)-spx;
        if (offset < 0 || offset > 7) {
          // This sprite does not cover this pixel.
          continue;
        }
        u8 b1 = sprite_fetched_data[s * 2];
        u8 b2 = sprite_fetched_data[s * 2 + 1];
        u8 b = 7 - (u8)offset;
        if (fetched_sprites[s].x_flip()) {
          b = (u8)offset;
        }
        u8 lo = (!!(b1 & (1 << b)));
        u8 hi = (!!(b2 & (1 << b))) << 1;
        u8 color = hi | lo;
        if (color == 0) {
          // If this sprite is transparent, we look for the next sprite to
          // blend.
          continue;
        }
        // Use this pixel.
        pixel.color = color;
        pixel.palette = fetched_sprites[s].dmg_palette() ? obp1 : obp0;
        pixel.bg_priority = fetched_sprites[s].priority();
        break;
      }
    }
    obj_queue.push_back(pixel);
  }
}