#ifndef __PLATFORM_H__
#define __PLATFORM_H__
#include "defs.h"
#include "emu.h"
#include<SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

class PLATFORM{
public:
  void init();
  void handle_events();
  void update(f64 dt);
  void run();
  void render();
  void clean();

  void update_main_window(EMU *emu);
  void update_dbg_window(EMU *emu);
  void display_tile(EMU *emu, SDL_Surface *surface, u16 startAddr, int tileNum, int x, int y);
  void check_key_down(SDL_Event event);
  void check_key_up(SDL_Event event);

  PLATFORM() = default;
  PLATFORM(int argc, char *argv[]);

private:
   constexpr static int SCREEN_WIDTH = 1200;//1024
   constexpr static int SCREEN_HEIGHT = 1080;//768

  int scale_ = 4;
   u32 tile_colors[4];

  SDL_Window *sdlWindow_;
  SDL_Renderer *sdlRenderer_;
  SDL_Texture *sdlTexture_;
  SDL_Surface *screen_;

  SDL_Window *sdlDebugWindow_;
  SDL_Renderer *sdlDebugRenderer_;
  SDL_Texture *sdlDebugTexture_;
  SDL_Surface *debugScreen_;
  EMU emu_;
  bool is_running_ = true; // 是否正在运行

   f64 TARGET_FPS = 59.73;
};
#endif // __PLATFORM_H__