#include"ppu.h"
#include <iostream>
#include"emu.h"
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
  // TODO
  if (line_cycles >= 80) {
    set_mode(PPUMode::drawing);
  }
}

void PPU::tick_drawing(EMU *emu)
{
    //TODO
    if(line_cycles >= 369){
        set_mode(PPUMode::hblank);
        if(hblank_int_enabled()){
            emu->int_flags |=INT_LCD_STAT;
        }
    }
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
            if(oam_int_enabled()){
                emu->int_flags |= INT_LCD_STAT;
            }
        }
        line_cycles = 0;
    }
}

void PPU::increase_ly(EMU *emu)
{
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
