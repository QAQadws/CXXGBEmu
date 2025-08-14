#include "platform.h"
#include "SDL3/SDL_keycode.h"
#include "defs.h"
#include <chrono>
#include <thread> 
#include<iostream>

inline u8 apply_palette(u8 color, u8 palette)
{
    switch(color)
    {
        case 0: color = palette & 0x03; break;
        case 1: color = ((palette >> 2) & 0x03); break;
        case 2: color = ((palette >> 4) & 0x03); break;
        case 3: color = ((palette >> 6) & 0x03); break;
        default: std::cerr << "Invalid color index" << std::endl; break;
    }
    return color;
}

void PLATFORM::init()
{
  TARGET_FPS = 59.73;
   tile_colors[0] = 0xFFFFFFFF; // 白色
  tile_colors[1] = 0xFFAAAAAA; // 灰色
  tile_colors[2] = 0xFF555555; // 深灰色
  tile_colors[3] = 0xFF000000; // 黑色
  scale_ = 4;
  is_running_ = true;
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    SDL_Log("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    is_running_ = false;
    }
    if(!TTF_Init()) {
      SDL_Log("SDL_ttf could not initialize! SDL_ttf Error: %s\n", SDL_GetError());
      is_running_ = false;
    }
    SDL_CreateWindowAndRenderer("GBEMU",SCREEN_WIDTH, SCREEN_HEIGHT, 0, &sdlWindow_, &sdlRenderer_);
    SDL_CreateWindowAndRenderer("DEBUG", 16*8*scale_, 32*8*scale_, 0, &sdlDebugWindow_, &sdlDebugRenderer_);
    debugScreen_ = SDL_CreateSurface( (16 * 8 * scale_), (32 * 8 * scale_) ,SDL_PixelFormat::SDL_PIXELFORMAT_ABGR8888);
    sdlDebugTexture_ = SDL_CreateTexture(sdlDebugRenderer_, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,debugScreen_->w, debugScreen_->h);
    int x,y;
    SDL_GetWindowPosition(sdlWindow_, &x, &y);
    SDL_SetWindowPosition(sdlDebugWindow_, x + SCREEN_WIDTH+10, y);

    //160 = 8 * 20 144 = 8 * 18
    screen_ = SDL_CreateSurface(20 * 8 * scale_, 18 * 8 * scale_, SDL_PixelFormat::SDL_PIXELFORMAT_ABGR8888);
    sdlTexture_ = SDL_CreateTexture(sdlRenderer_, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, screen_->w, screen_->h);
}

void PLATFORM::handle_events()
{
  SDL_Event event;
  while(SDL_PollEvent(&event)){
    switch(event.type){
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        is_running_ = false;
        break;
      case SDL_EVENT_KEY_DOWN:
        check_key_down(event);
        break;
      case SDL_EVENT_KEY_UP:
        check_key_up(event);
        break;
    }
  }
}



void PLATFORM::update(f64 dt) {
    emu_.update(dt);
    update_dbg_window(&emu_);
    update_main_window(&emu_);
}

void PLATFORM::run() {
  const auto TARGET_FRAME_TIME =std::chrono::duration<double>(1.0 / TARGET_FPS);
  auto lastFrameTime = std::chrono::high_resolution_clock::now();
  int frame_count = 0;
  auto time1 = std::chrono::high_resolution_clock::now();

  while (is_running_) {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto elapsed = currentTime - lastFrameTime;

    if (elapsed >= TARGET_FRAME_TIME) {
      handle_events();
      update(std::chrono::duration<double>(elapsed).count());
      render();
      frame_count++;
      lastFrameTime = currentTime;
    } else {
      // ✅ 混合延迟策略
      auto remaining = TARGET_FRAME_TIME - elapsed;

      if (remaining > std::chrono::milliseconds(2)) {
        // 剩余时间>2ms时，使用SDL延迟减少CPU占用
        auto delay_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            remaining - std::chrono::milliseconds(1));
        SDL_DelayNS(delay_ns.count());
      } else if (remaining > std::chrono::microseconds(100)) {
        // 剩余时间100μs-2ms时，使用yield
        std::this_thread::yield();
      }
      // 剩余时间<100μs时，忙等待保证精度
    }

    if (frame_count == 600) {
      auto time2 = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsed = time2 - time1;
      std::cout << "Frame rate: " << frame_count / elapsed.count() << " FPS"
                << std::endl;
      frame_count = 0;
      time1 = std::chrono::high_resolution_clock::now();
    }
  }
}

void PLATFORM::render() {
    SDL_UpdateTexture(sdlDebugTexture_, NULL, debugScreen_->pixels, debugScreen_->pitch);
    SDL_RenderClear(sdlDebugRenderer_);
    SDL_RenderTexture(sdlDebugRenderer_, sdlDebugTexture_, NULL, NULL);
    SDL_RenderPresent(sdlDebugRenderer_);

    SDL_UpdateTexture(sdlTexture_, NULL, screen_->pixels, screen_->pitch);
    SDL_RenderClear(sdlRenderer_);
    SDL_RenderTexture(sdlRenderer_, sdlTexture_, NULL, NULL);
    SDL_RenderPresent(sdlRenderer_);
}

void PLATFORM::clean() {
    // TODO
    emu_.clean();
}

void PLATFORM::update_main_window(EMU *emu)
{ //RGBA ARGB
  SDL_Rect rc1;
  rc1.x = 0;
  rc1.y = 0;
  rc1.w = screen_->w;
  rc1.h = screen_->h;
  SDL_FillSurfaceRect(screen_, &rc1, 0xFF111111);
  u8 * pixels = emu->ppu_.pixels + (emu->ppu_.current_back_buffer * PPU_XRES * PPU_YRES * 4);
  for(int y =0; y < PPU_YRES; y++){
    for(int x = 0; x < PPU_XRES; x++){
      u32 color = ((u32)(pixels[(y * PPU_XRES + x) * 4])<<16) +//R
                  ((u32)(pixels[(y * PPU_XRES + x) * 4 + 1])<<8) +//G
                  ((u32)(pixels[(y * PPU_XRES + x) * 4 + 2])) +//B
                  ((u32)(pixels[(y * PPU_XRES + x) * 4 + 3])<<24); //A
      SDL_Rect rc;
      rc.x = x * scale_;
      rc.y = y * scale_;
      rc.w = scale_;
      rc.h = scale_;
      SDL_FillSurfaceRect(screen_, &rc, color);
    }
  }
}

void PLATFORM::update_dbg_window(EMU *emu)
{

  int tileNum = 0;
  SDL_Rect rc;
  rc.x = 0;
  rc.y = 0;
  rc.w = debugScreen_->w;
  rc.h = debugScreen_->h;
  SDL_FillSurfaceRect(debugScreen_, &rc,0xFF111111);
  u16 addr =0x8000;
  //16*24 = 384
  int xDraw = 0;
  int yDraw = 0;
  for(int y = 0; y < 24; y++) {
    for(int x = 0; x < 16; x++) {
      display_tile(emu, debugScreen_, addr, tileNum,xDraw, yDraw);
      xDraw += (8 * scale_);
      tileNum++;
    }
    yDraw += (8 * scale_);
    xDraw = 0;
  }
}

void PLATFORM::display_tile(EMU *emu, SDL_Surface *surface, u16 startAddr, int tileNum, int x, int y)
{
  SDL_Rect rc;
  for(int tileY =0 ;tileY<16;tileY+=2){
    u8 b1 =emu->bus_read(startAddr + tileY + (tileNum*16));
    u8 b2 =emu->bus_read(startAddr + tileY + (tileNum*16) + 1);
    for(int bit =7;bit>=0;--bit){
      u8 lo =!!(b2 & (1 << bit))<<1;
      u8 hi = !!(b1 & (1 << bit));
      u8 color = hi|lo;
      rc.x = x + ((7-bit)*scale_);
      rc.y = y + ((tileY/2)*scale_);
      rc.w = scale_;
      rc.h = scale_;
      SDL_FillSurfaceRect(surface, &rc, tile_colors[color]);
    }
  }
}

void PLATFORM::check_key_down(SDL_Event event)
{
  switch (event.key.key){
    case SDLK_ESCAPE:
      is_running_ = false;
      break;
    case SDLK_A:
      emu_.joypad_.left = true;
      break;
    case SDLK_D:
      emu_.joypad_.right = true;
      break;
    case SDLK_W:
      emu_.joypad_.up = true;
      break;
    case SDLK_S:
      emu_.joypad_.down = true;
      break;
    case SDLK_J:
      emu_.joypad_.a = true;
      break;
    case SDLK_K:
      emu_.joypad_.b = true;
      break;
    case SDLK_U:
      emu_.joypad_.select = true;
      break;
    case SDLK_I:
      emu_.joypad_.start = true;
      break;
  }
}

void PLATFORM::check_key_up(SDL_Event event)
{
  switch (event.key.key){
    case SDLK_A:
      emu_.joypad_.left = false;
      break;
    case SDLK_D:
      emu_.joypad_.right = false;
      break;
    case SDLK_W:
      emu_.joypad_.up = false;
      break;
    case SDLK_S:
      emu_.joypad_.down = false;
      break;
    case SDLK_J:
      emu_.joypad_.a = false;
      break;
    case SDLK_K:
      emu_.joypad_.b = false;
      break;
    case SDLK_U:
      emu_.joypad_.select = false;
      break;
    case SDLK_I:
      emu_.joypad_.start = false;
      break;
  }
}



PLATFORM::PLATFORM(int argc, char *argv[]): emu_(argc, argv), is_running_(true) {
    // 初始化其他成员变量或资源
}

