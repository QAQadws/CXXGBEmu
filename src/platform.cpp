#include "platform.h"
#include "defs.h"
#include <chrono>
#include <thread> //24*16 = 384



void PLATFORM::init()
{
  tile_colors[3] = 0xFFFFFFFF; // 白色
  tile_colors[2] = 0xFFAAAAAA; // 灰色
  tile_colors[1] = 0xFF555555; // 深灰色
  tile_colors[0] = 0xFF000000; // 黑色
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
    debugScreen_ = SDL_CreateSurface( (16 * 8 * scale_) + (16 * scale_), (32 * 8 * scale_) + (64 * scale_),SDL_PixelFormat::SDL_PIXELFORMAT_ABGR8888);
    sdlDebugTexture_ = SDL_CreateTexture(sdlDebugRenderer_, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,(16 * 8 * scale_) + (16 * scale_), (32 * 8 * scale_) + (64 * scale_));
    int x,y;
    SDL_GetWindowPosition(sdlWindow_, &x, &y);
    SDL_SetWindowPosition(sdlDebugWindow_, x + SCREEN_WIDTH+10, y);
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
        switch(event.key.key){
          case SDLK_ESCAPE:
            is_running_ = false;
            break;
        }
        break;
    }
  }
}



void PLATFORM::update(f64 dt) {
    emu_.update(dt);
    update_dbg_window(&emu_);
}

void PLATFORM::run() {
  const f64 TARGET_FPS = 59.73;
  const auto TARGET_FRAME_TIME = std::chrono::duration<double>(1.0 / TARGET_FPS);

  auto lastFrameTime = std::chrono::high_resolution_clock::now();

  while (is_running_) {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto elapsed = currentTime - lastFrameTime;

    if (elapsed >= TARGET_FRAME_TIME) {
      f64 deltaTime = std::chrono::duration<double>(elapsed).count();

      handle_events();
      update(deltaTime);
      render();

      lastFrameTime = currentTime;
    } else {
      // 短暂休眠避免 CPU 100% 占用
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
  }

}

void PLATFORM::render() {
    SDL_UpdateTexture(sdlDebugTexture_, NULL, debugScreen_->pixels, debugScreen_->pitch);
    SDL_RenderClear(sdlDebugRenderer_);
    SDL_RenderTexture(sdlDebugRenderer_, sdlDebugTexture_, NULL, NULL);
    SDL_RenderPresent(sdlDebugRenderer_);
}

void PLATFORM::clean() {
    // TODO
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
      u8 hi =!!(b2 & (1 << bit))<<1;
      u8 lo = !!(b1 & (1 << bit));
      u8 color = hi|lo;
      rc.x = x + ((7-bit)*scale_);
      rc.y = y + ((tileY/2)*scale_);
      rc.w = scale_;
      rc.h = scale_;
      SDL_FillSurfaceRect(surface, &rc, tile_colors[color]);
    }
  }
}



PLATFORM::PLATFORM(int argc, char *argv[]): emu_(argc, argv), is_running_(true) {
    // 初始化其他成员变量或资源
}

