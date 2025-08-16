#ifndef __PLATFORM_H__
#define __PLATFORM_H__
#include "defs.h"
#include "emu.h"
#include<SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include<mutex>
#include<deque>


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

   // ✅ 添加音频相关成员
   SDL_AudioStream *audio_stream_;
   SDL_AudioDeviceID audio_device_id_;
   std::deque<f64> audio_buffer_l_; // 音频缓冲区l
   std::deque<f64> audio_buffer_r_; // 音频缓冲区r
   mutable std::mutex audio_buffer_lock_;

  void init_audio();
  void cleanup_audio();
  static void audio_callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);
  void push_audio_sample(float left, float right);
};
#endif // __PLATFORM_H__