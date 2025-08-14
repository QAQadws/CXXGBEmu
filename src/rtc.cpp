#include "rtc.h"
void RTC::init()
{
    s = 0;  // Seconds.
    m = 0;  // Minutes.
    h = 0;  // Hours.
    dl = 0; // Lower 8 bits of Day Counter.
    dh = 0; // Upper 1 bit of Day Counter, Carry Bit, Halt Flag.

  // Internal state.
   time = 0;
  time_latched = false;
  // Set to `true` when writing 0x00 to 0x6000~0x7FFF.
  time_latching = false;
}

void RTC::update(f64 delta_time)
{
  if (!halted()) {
    time += delta_time;
    if (!time_latched) {
      update_time_registers();
    }
  }
}
void RTC::update_time_registers() {
  s = ((u64)time) % 60;
  m = (((u64)time) / 60) % 60;
  h = (((u64)time) / 3600) % 24;
  u16 days = (u16)(((u64)time) / 86400);
  dl = (u8)(days & 0xFF);
  if (days & 0x100)
    dh |= 0x01;
  else
    dh &= ~0x01;
  if (days >= 512)
    dh |= 0x80;
  else
    dh &= ~0x80;
}
void RTC::update_timestamp() {
  time = s + ((u64)m) * 60 + ((u64)h) * 3600 + ((u64)days()) * 86400;
  if (day_overflow()) {
    time += 86400 * 512;
  }
}
void RTC::latch() {
  if (!time_latched) {
    time_latched = true;
  } else {
    time_latched = false;
    update_time_registers();
  }
}