#include"apu.h"
#include <cstring>
#include <iostream>
#include"emu.h"
#include"platform.h"

// 在 apu.cpp 文件顶部添加这些常量
constexpr u8 pulse_wave_0[8] = {1, 1, 1, 1, 1, 1, 1, 0};
constexpr u8 pulse_wave_1[8] = {0, 1, 1, 1, 1, 1, 1, 0};
constexpr u8 pulse_wave_2[8] = {0, 1, 1, 1, 1, 0, 0, 0};
constexpr u8 pulse_wave_3[8] = {1, 0, 0, 0, 0, 0, 0, 1};

// DAC转换函数
inline float dac(u8 sample) {
  // 线性插值：将0-15的数字样本转换为-1.0到1.0的浮点音频样本
  return -1.0f + (2.0f * (15 - sample)) / 15.0f;
}

void APU::disable() { 
    std::memset(this, 0, sizeof(*this)); 
}

void APU::enable_ch1()
{
  //bit_set(&nr52_master_control, 0);
  nr52_master_control |= 0x01;
  // Save NR1x states to registers.
  ch1_sample_index = 0;
  ch1_volume = ch1_initial_volume();
  ch1_period_counter = ch1_period();
  ch1_volume = ch1_initial_volume();
  ch1_sweep_iteration_counter = 0;
  ch1_sweep_iteration_pace = ch1_sweep_pace();
  ch1_envelope_iteration_increase = ch1_envelope_increase();
  ch1_envelope_iteration_pace = ch1_envelope_pace();
  ch1_envelope_iteration_counter = 0;
  ch1_length_timer = ch1_initial_length_timer();
}

void APU::disable_ch1()
{
    nr52_master_control &= (~0x01);
}

void APU::enable_ch2()
{
  nr52_master_control |= 0x02; // bit_set(&nr52_master_control, 1);
  ch2_sample_index = 0;
  ch2_volume = ch2_initial_volume();
  ch2_period_counter = 0;
  ch2_envelope_iteration_increase = ch2_envelope_increase();
  ch2_envelope_iteration_pace = ch2_envelope_pace();
  ch2_envelope_iteration_counter = 0;
  ch2_length_timer = ch2_initial_length_timer();
}

void APU::disable_ch2()
{
    nr52_master_control &= (~0x02); // bit_reset(&nr52_master_control, 1);
}

void APU::tick_ch1_sweep()
{
  if (ch1_enabled() && ch1_sweep_pace()) {
    ++ch1_sweep_iteration_counter;
    if (ch1_sweep_iteration_counter == ch1_sweep_iteration_pace) {
      // Computes period after modification.
      s32 period = (s32)ch1_period();
      u8 step = ch1_sweep_individual_step();
      if (ch1_sweep_subtraction()) {
        period -= period / (1 << step);
      } else {
        period += period / (1 << step);
      }
      // If period is out of valid range after sweep, the channel is
      // disabled.
      if (period > 0x07FF || period <= 0) {
        disable_ch1();
      } else {
        // Write period back.
        set_ch1_period((u16)period);
      }
      ch1_sweep_iteration_counter = 0;
      // Reload iteration pace when one iteration completes.
      ch1_sweep_iteration_pace = ch1_sweep_pace();
    }
  }
}

void APU::tick_ch1_envelope()
{
  if (ch1_enabled() && ch1_envelope_iteration_pace) {
    ++ch1_envelope_iteration_counter;
    if (ch1_envelope_iteration_counter >= ch1_envelope_iteration_pace) {
      if (ch1_envelope_iteration_increase) {
        if (ch1_volume < 15) {
          ++ch1_volume;
        }
      } else {
        if (ch1_volume > 0) {
          --ch1_volume;
        }
      }
      ch1_envelope_iteration_counter = 0;
    }
  }
}

void APU::tick_ch1_length()
{
  if (ch1_enabled() && ch1_length_enabled()) {
    ++ch1_length_timer;
    if (ch1_length_timer >= 64) {
      disable_ch1();
    }
  }
}

void APU::tick_ch1(EMU *emu)
{
  // 检查CH1的DAC是否开启
  if (!ch1_dac_on()) {
    disable_ch1();
    return;
  }

  // 递增周期计数器
  ++ch1_period_counter;

  // 当计数器达到阈值时，前进到下一个样本
  if (ch1_period_counter >= 0x800) {
    // 前进到下一个样本（8个样本循环）
    ch1_sample_index = (ch1_sample_index + 1) % 8;
    // 重置计数器为当前周期值
    ch1_period_counter = ch1_period();
  }

  // 根据波形类型获取当前样本
  u8 sample = 0;
  switch (ch1_wave_type()) {
  case 0:
    sample = pulse_wave_0[ch1_sample_index];
    break;
  case 1:
    sample = pulse_wave_1[ch1_sample_index];
    break;
  case 2:
    sample = pulse_wave_2[ch1_sample_index];
    break;
  case 3:
    sample = pulse_wave_3[ch1_sample_index];
    break;
  default:
    break;
  }

  // 生成最终的输出样本（样本值 × 音量）
  ch1_output_sample = dac(sample * ch1_volume);
}

void APU::tick_ch2_envelope()
{
  if (ch2_enabled() && ch2_envelope_iteration_pace) {
    ++ch2_envelope_iteration_counter;
    if (ch2_envelope_iteration_counter >= ch2_envelope_iteration_pace) {
      if (ch2_envelope_iteration_increase) {
        if (ch2_volume < 15) {
          ++ch2_volume;
        }
      } else {
        if (ch2_volume > 0) {
          --ch2_volume;
        }
      }
      ch2_envelope_iteration_counter = 0;
    }
  }
}

void APU::tick_ch2_length()
{
  if (ch2_enabled() && ch2_length_enabled()) {
    ++ch2_length_timer;
    if (ch2_length_timer >= 64) {
      disable_ch2();
    }
  }
}

void APU::tick_ch2(EMU *emu)
{
  if (!ch2_dac_on()) {
    disable_ch2();
    return;
  }
  
  ++ch2_period_counter;
  if (ch2_period_counter >= 0x800) {
    // advance to next sample.
    ch2_sample_index = (ch2_sample_index + 1) % 8;
    ch2_period_counter = ch2_period();
  }
  
  u8 sample = 0;
  switch (ch2_wave_type()) {
  case 0:
    sample = pulse_wave_0[ch2_sample_index];
    break;
  case 1:
    sample = pulse_wave_1[ch2_sample_index];
    break;
  case 2:
    sample = pulse_wave_2[ch2_sample_index];
    break;
  case 3:
    sample = pulse_wave_3[ch2_sample_index];
    break;
  default:
    break;
  }
  
  ch2_output_sample = dac(sample * ch2_volume);
}

void APU::init()
{
    std::memset(this, 0, sizeof(*this));
}

void APU::tick(EMU *emu)
{
    if(!is_enabled()) return;
    tick_div_apu(emu);
    if ((emu->clock_cycles_ % 4) == 0) {
      // Tick CH1.
      if (ch1_enabled()) {
        tick_ch1(emu);
      }
      // Tick CH2.
      if (ch2_enabled()) {
        tick_ch2(emu);
      }
      output_audio_samples(emu);
    }
}

u8 APU::bus_read(u16 addr)
{
  // CH1 registers.
  if (addr >= 0xFF10 && addr <= 0xFF14) {
    if (addr == 0xFF11) {
      // lower 6 bits of NR11 is write-only.
      return nr11_ch1_length_timer_duty_cycle & 0xC0;
    }
    if (addr == 0xFF14) {
      // only bit 6 is readable.
      return nr14_ch1_period_high_control & 0x40;
    }
    return (&nr10_ch1_sweep)[addr - 0xFF10];
  }
  // CH2 registers.
  if (addr >= 0xFF16 && addr <= 0xFF19) {
    if (addr == 0xFF16) {
      // lower 6 bits of NR21 is write-only.
      return nr21_ch2_length_timer_duty_cycle & 0xC0;
    }
    if (addr == 0xFF19) {
      // only bit 6 is readable.
      return nr24_ch2_period_high_control & 0x40;
    }
    return (&nr21_ch2_length_timer_duty_cycle)[addr - 0xFF16];
  }
  // Master control registers.
  if (addr >= 0xFF24 && addr <= 0xFF26) {
    return (&nr50_master_volume_vin_panning)[addr - 0xFF24];
  }
  std::cerr << "APU: Invalid read from " << std::hex << addr << std::endl;
  return 0xFF;
}

void APU::bus_write(u16 addr, u8 data)
{
  // CH1 registers.
  if (addr >= 0xFF10 && addr <= 0xFF14) {
    if (!is_enabled()) {
      // Only NRx1 is writable.
      if (addr == 0xFF11) {
        nr11_ch1_length_timer_duty_cycle = data;
      }
    } else {
      if (addr == 0xFF10) {
        if ((nr10_ch1_sweep & 0x70) == 0 && ((data & 0x70) != 0)) {
          // Restart sweep iteration.
          ch1_sweep_iteration_counter = 0;
          ch1_sweep_iteration_pace = (data & 0x70) >> 4;
        }
      }
      if (addr == 0xFF14 && (data & 0x80)) { // bit_test(&data, 7)
        // CH1 trigger.
        enable_ch1();
        data &= 0x7F;
      }
      (&nr10_ch1_sweep)[addr - 0xFF10] = data;
    }
    return;
  }
  // CH2 registers.
  if (addr >= 0xFF16 && addr <= 0xFF19) {
    if (!is_enabled()) {
      // Only NRx1 is writable.
      if (addr == 0xFF16) {
        nr21_ch2_length_timer_duty_cycle = data;
      }
    } else {
      if (addr == 0xFF19 && (data & 0x80)) { // bit_test(&data, 7)
        // CH2 trigger.
        enable_ch2();
        data &= 0x7F;
      }
      (&nr21_ch2_length_timer_duty_cycle)[addr - 0xFF16] = data;
    }
    return;
  }
  // Master control registers.
  if (addr >= 0xFF24 && addr <= 0xFF26) {
    if (addr == 0xFF26) {
      bool enabled_before = is_enabled();
      // Only bit 7 is writable.
      nr52_master_control = (data & 0x80) | (nr52_master_control & 0x7F);
      if (enabled_before && !is_enabled()) {
        disable();
      }
      return;
    }
    // All registers except NR52 is read-only if APU is not enabled.
    if (!is_enabled())
      return;
    (&nr50_master_volume_vin_panning)[addr - 0xFF24] = data;
    return;
  }
  std::cerr << "APU: Unsupported bus write address: 0x" << std::hex << addr << std::endl;
}

void APU::output_audio_samples(EMU *emu)
{
  // ✅ Mixer - 音频混合器
  // Output volume range in [-4, 4].
  float sample_l = 0.0f;
  float sample_r = 0.0f;

  // ✅ 混合CH1的左右声道输出
  if (ch1_dac_on() && ch1_l_enabled()) {
    sample_l += ch1_output_sample;
  }
  if (ch1_dac_on() && ch1_r_enabled()) {
    sample_r += ch1_output_sample;
  }

  // ✅ 混合CH2的左右声道输出
  if (ch2_dac_on() && ch2_l_enabled()) {
    sample_l += ch2_output_sample;
  }
  if (ch2_dac_on() && ch2_r_enabled()) {
    sample_r += ch2_output_sample;
  }

  // TODO: 当实现其他声道时，在这里添加CH3, CH4的混合
  // if (ch3_dac_on() && ch3_l_enabled()) sample_l += ch3_output_sample;
  // if (ch3_dac_on() && ch3_r_enabled()) sample_r += ch3_output_sample;
  // ...

  // ✅ Volume control - 音量控制
  // Scale output volume to [-1, 1].
  sample_l /= 4.0f;
  sample_r /= 4.0f;
  sample_l *= static_cast<float>(left_volume()) / 7.0f;
  sample_r *= static_cast<float>(right_volume()) / 7.0f;

  // ✅ Output samples - 输出样本到音频缓冲区
  if (emu->platform_) {
    emu->platform_->push_audio_sample(sample_l, sample_r);
  }
}


void APU::tick_div_apu(EMU *emu)
{
  u8 div = emu->timer_.read_div();
  // When DIV bit 4 goes from 1 to 0...
  if ((last_div & 0x10) && !(div & 0x10)) {
    // 512Hz.
    ++div_apu;
    if ((div_apu % 2) == 0) {
      // Length is ticked at 256Hz.
      tick_ch1_length();
      tick_ch2_length();
    }
    if ((div_apu % 4) == 0) {
      // Sweep is ticked at 128Hz.
      tick_ch1_sweep();
    }
    if ((div_apu % 8) == 0) {
      // Envelope is ticked at 64Hz.
      tick_ch1_envelope();
      tick_ch2_envelope();
    }
  }
  last_div = div;
}
