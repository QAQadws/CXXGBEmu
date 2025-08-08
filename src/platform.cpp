#include "platform.h"
#include "defs.h"
#include <chrono>
//#include <thread>

void PLATFORM::init(int argc, char *argv[]) {
    emu_ = EMU(argc, argv);
    is_running_ = true;
}

void PLATFORM::event() {
    // TODO
}

void PLATFORM::update(f64 dt) {
    emu_.update(dt);
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

      event();
      update(deltaTime);
      render();

      lastFrameTime = currentTime;
    } else {
      // 短暂休眠避免 CPU 100% 占用
      //std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
  }
}

void PLATFORM::render() {
    // TODO
}

void PLATFORM::clean() {
    // TODO
}
