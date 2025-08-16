#include "platform.h"
#include "defs.h"
#include <chrono>
#include <thread> 
#include<iostream>
#include <algorithm>
#include <cmath>

// ✅ 线性插值函数
inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

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

    SDL_CreateWindowAndRenderer("GBEMU",SCREEN_WIDTH, SCREEN_HEIGHT, 0, &sdlWindow_, &sdlRenderer_);
    SDL_CreateWindowAndRenderer("DEBUG", 16*8*scale_, 32*8*scale_, 0, &sdlDebugWindow_, &sdlDebugRenderer_);
    
    // ✅ 启用VSync以防止撕裂
    SDL_SetRenderVSync(sdlRenderer_, 1);
    SDL_SetRenderVSync(sdlDebugRenderer_, 1);
    
    debugScreen_ = SDL_CreateSurface( (16 * 8 * scale_), (32 * 8 * scale_) ,SDL_PixelFormat::SDL_PIXELFORMAT_ABGR8888);
    sdlDebugTexture_ = SDL_CreateTexture(sdlDebugRenderer_, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,debugScreen_->w, debugScreen_->h);
    int x,y;
    SDL_GetWindowPosition(sdlWindow_, &x, &y);
    SDL_SetWindowPosition(sdlDebugWindow_, x + SCREEN_WIDTH+10, y);

    //160 = 8 * 20 144 = 8 * 18
    screen_ = SDL_CreateSurface(20 * 8 * scale_, 18 * 8 * scale_, SDL_PixelFormat::SDL_PIXELFORMAT_ABGR8888);
    sdlTexture_ = SDL_CreateTexture(sdlRenderer_, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, screen_->w, screen_->h);

    init_audio();
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

    if (frame_count == 1800) {
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
    cleanup_audio();
    emu_.clean();
    SDL_Quit();
}

void PLATFORM::update_main_window(EMU *emu)
{ //RGBA ARGB
  SDL_Rect rc1;
  rc1.x = 0;
  rc1.y = 0;
  rc1.w = screen_->w;
  rc1.h = screen_->h;
  SDL_FillSurfaceRect(screen_, &rc1, 0xFF111111);
  
  // ✅ 使用新的显示缓冲区获取方法
  u8 * pixels = emu->ppu_.get_display_buffer();
  
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
    audio_stream_ = nullptr;
    audio_device_id_ = 0;
    emu_.platform_ = this;
}

void PLATFORM::init_audio() {
  // ✅ 配置音频规格 - 对应Luna SDK的设置
  SDL_AudioSpec spec;
  spec.format = SDL_AUDIO_F32; // 32位浮点 (对应 BitDepth::f32)
  spec.channels = 2;           // 立体声 (对应 num_channels = 2)
  spec.freq = 44100;           // 44.1kHz采样率

  // ✅ 打开音频设备流 (对应 new_device)
  audio_stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                            &spec, audio_callback, this);

  if (!audio_stream_) {
    SDL_Log("Failed to open audio device: %s", SDL_GetError());
    return;
  }

  // ✅ 获取音频设备ID (对应选择primary adapter)
  audio_device_id_ = SDL_GetAudioStreamDevice(audio_stream_);

  // ✅ 启动音频设备 (对应添加回调)
  SDL_ResumeAudioStreamDevice(audio_stream_);

  SDL_Log("Audio initialized: %d Hz, %d channels, format=%d", spec.freq,
          spec.channels, spec.format);

}

void PLATFORM::audio_callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount)
{
    PLATFORM* platform = static_cast<PLATFORM*>(userdata);
    
    const int channels = 2;
    const int bytes_per_sample = sizeof(float);
    int frames_needed = additional_amount / (channels * bytes_per_sample);
    
    if (frames_needed <= 0) return;
    
    std::vector<float> audio_buffer(frames_needed * channels);
    
    // ✅ 音频重采样和输出
    std::lock_guard<std::mutex> guard(platform->audio_buffer_lock_);
    
    // 模拟器采样率：1048576 Hz，音频设备采样率：44100 Hz
    const double EMULATOR_SAMPLE_RATE = 1048576.0;
    const double DEVICE_SAMPLE_RATE = 44100.0;  // 应该从实际设备获取
    
    int num_frames_read = 0;
    
    while (num_frames_read < frames_needed) {
        // ✅ 计算当前帧在音频时间线上的时间戳
        double timestamp = static_cast<double>(num_frames_read) / DEVICE_SAMPLE_RATE;
        
        // ✅ 将时间戳转换为模拟器音频缓冲区中的索引
        double sample_index = timestamp * EMULATOR_SAMPLE_RATE;
        
        // ✅ 获取前后两个采样点的索引
        size_t sample_1_index = static_cast<size_t>(std::floor(sample_index));
        size_t sample_2_index = static_cast<size_t>(std::ceil(sample_index));
        
        // ✅ 检查缓冲区边界
        if (sample_2_index >= platform->audio_buffer_l_.size() || 
            sample_2_index >= platform->audio_buffer_r_.size()) {
            break;
        }
        
        // ✅ 获取左右声道的前后采样点
        float sample_1_l = static_cast<float>(platform->audio_buffer_l_[sample_1_index]);
        float sample_2_l = static_cast<float>(platform->audio_buffer_l_[sample_2_index]);
        float sample_1_r = static_cast<float>(platform->audio_buffer_r_[sample_1_index]);
        float sample_2_r = static_cast<float>(platform->audio_buffer_r_[sample_2_index]);
        
        // ✅ 线性插值因子
        float interpolation_factor = static_cast<float>(sample_index) - static_cast<float>(sample_1_index);
        
        // ✅ 执行线性插值 (lerp)
        float output_left = lerp(sample_1_l, sample_2_l, interpolation_factor);
        float output_right = lerp(sample_1_r, sample_2_r, interpolation_factor);
        
        // ✅ 写入输出缓冲区
        audio_buffer[num_frames_read * 2] = output_left;      // 左声道
        audio_buffer[num_frames_read * 2 + 1] = output_right; // 右声道
        
        ++num_frames_read;
    }
    
    // ✅ 从模拟器音频缓冲区中移除已处理的数据
    if (num_frames_read > 0) {
        // 计算输出音频片段的总时长
        double delta_time = static_cast<double>(num_frames_read) / DEVICE_SAMPLE_RATE;
        
        // 计算需要移除的采样数量
        size_t num_samples_to_remove = static_cast<size_t>(delta_time * EMULATOR_SAMPLE_RATE);
        
        // 限制移除数量不超过缓冲区大小
        num_samples_to_remove = std::min(num_samples_to_remove, platform->audio_buffer_l_.size());
        num_samples_to_remove = std::min(num_samples_to_remove, platform->audio_buffer_r_.size());
        
        // ✅ 从缓冲区头部移除已处理的数据
        for (size_t i = 0; i < num_samples_to_remove; ++i) {
            if (!platform->audio_buffer_l_.empty()) platform->audio_buffer_l_.pop_front();
            if (!platform->audio_buffer_r_.empty()) platform->audio_buffer_r_.pop_front();
        }
    }
    
    // ✅ 将处理后的音频数据送入SDL音频流
    SDL_PutAudioStreamData(stream, audio_buffer.data(), num_frames_read * channels * bytes_per_sample);
}

void PLATFORM::push_audio_sample(float left, float right)
{
  {
    std::lock_guard<std::mutex> guard(audio_buffer_lock_);

    // ✅ 限制音频缓冲区大小，最多存储65536个样本
    // (约十六分之一秒的音频数据)
    constexpr size_t AUDIO_BUFFER_MAX_SIZE = 65536;

    if (audio_buffer_l_.size() >= AUDIO_BUFFER_MAX_SIZE) {
      audio_buffer_l_.pop_front();
    }
    if (audio_buffer_r_.size() >= AUDIO_BUFFER_MAX_SIZE) {
      audio_buffer_r_.pop_front();
    }

    audio_buffer_l_.push_back(left);
    audio_buffer_r_.push_back(right);
  }
}
void PLATFORM::cleanup_audio() {
  if (audio_stream_) {
    SDL_CloseAudioDevice(audio_device_id_);
    audio_stream_ = nullptr;
  }
}
