#ifndef __PLATFORM_H__
#define __PLATFORM_H__
#include "defs.h"
#include "emu.h"
class PLATFORM{
public:
  void init(int argc, char *argv[]);
  void event();
  void update(f64 dt);
  void run();
  void render();
  void clean();
  
  PLATFORM() = default;

private:
  EMU emu_;
  bool is_running_ = true; // 是否正在运行
};
#endif // __PLATFORM_H__