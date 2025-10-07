#include "serial.h"
#include "emu.h"
#include "link.h"
#include <iostream>


void SERIAL::init() {
  sb_ = 0xFF;
  sc_ = 0x7C; // 0111 1100: 仅bit7/bit1/bit0有意义，其它读作1
  transferring_ = false;
  have_rx_ = false;
  sb_latched_ = 0xFF;
  rx_latched_ = 0xFF;
}

void SERIAL::tick(EMU *emu) {
  // 网络收包：把对端发来的本轮字节取走
  if (link_) {
    u8 b;
    while (link_->try_recv(b)) {
      rx_latched_ = b;
      have_rx_ = true;
    }
  }

  // 主机（内部时钟）：若SC.7已经置1且还没开始，则立即开始
  if (!transferring_ && is_master() && transfer_enable()) {
    begin_transfer();
  }

  // 处理传输完成条件（字节级锁步简化）
  if (transferring_) {
    if (link_ && link_->connected()) {
      if (is_master()) {
        // 主机：等到拿到对端字节即可完成
        if (have_rx_) {
          end_transfer(emu);
        }
      } else {
        // 从机：只有在收到主机字节后才开始；开始后立即完成
        if (have_rx_) {
          // 确保已经被SC.7上升沿触发（transfer_enable为真），再开始并结束
          if (!transferring_){
            begin_transfer();}
          end_transfer(emu);
        }
      }
    } else {
      // 未连接或断开
    //   if (is_master()) {
    //     // 断线场景主机读到 0xFF
    //     rx_latched_ = 0xFF;
    //     have_rx_ = true;
    //     end_transfer(emu);
    //   } else {
    //     // 外部时钟下应当等待时钟（不超时完成）
    //     // 可选：加一个超时以避免永不返回（这里选择严格等待）
    //   }
    }
  }
}

void SERIAL::begin_transfer() {
  transfer_bit_ = 7;
  out_byte_ = sb_latched_; // 只用锁存字节
  transferring_ = true;
}

void SERIAL::end_transfer(EMU *emu) {
  // 完成一字节交换：SB 读出为对端发来的字节
  sb_ = have_rx_ ? rx_latched_ : 0xFF;

  // 输出缓冲仅用于观测本端发出的字节
  output_buffer_.push_back(out_byte_);

  transferring_ = false;
  have_rx_ = false;
  sc_ &= 0x7F;                  // 清传输使能位
  emu->int_flags |= INT_SERIAL; // 触发串口中断
}

u8 SERIAL::bus_read(u16 address) {
  if (address == 0xFF01) {
    return sb_;
  } else if (address == 0xFF02) {
    return sc_;
  }
  std::cerr << "[SERIAL] warn: Invalid read at 0x" << std::hex << address
            << std::dec << "\n";
  return 0xFF;
}

void SERIAL::bus_write(u16 address, u8 data) {
  if (address == 0xFF01) {
    sb_ = data;
    return;
  } else if (address == 0xFF02) {
    // 仅允许写 bit7(启动) 与 bit0(主从), bit1 为倍速（CGB）；其余读作1
    u8 old = sc_;
    sc_ = 0x7C | (data & 0x83);

    bool old_en = (old & 0x80) != 0;
    bool new_en = (sc_ & 0x80) != 0;
    bool rising = ((!old_en) && new_en);

    if (rising) {
      // 在上升沿锁存本轮要发送的字节
      sb_latched_ = sb_;
      rx_latched_ = 0xFF;
      have_rx_ = false;

      // 把本轮字节发给对端
      if (link_ && link_->connected()) {
        link_->send(sb_latched_);
      }

      // 若是主机，立即进入传输态；从机等待收到主机字节再开始
      if (is_master()) {
        begin_transfer();
      } else {
        // 从机：保持transfer_enable为1，等待主机字节到来
        transferring_ = true; // 标记“本轮正在等待外部时钟”
      }
    }
    return;
  }

  std::cerr << "[SERIAL] warn: Invalid write at 0x" << std::hex << address
            << std::dec << "\n";
}

// #include "serial.h"
// #include "emu.h"
// #include <cstdlib>
// #include<iostream>
// void SERIAL::init()
// {
//     sb_ = 0xFF; 
//     sc_ = 0x7C; // 0x7C = 0111 1100
//     transferring_ = false;
// }

// void SERIAL::tick(EMU* emu)
// {   
//     if(!transferring_ && is_master() && transfer_enable()){
//         begin_transfer(); // 如果是主机并且传输使能，开始传
//     }
//     else if(transferring_){
//         process_transfer(emu); // 如果正在传输，处理传输
//     }
// }

// void SERIAL::begin_transfer()
// {
//     transfer_bit_ = 7; // 从最高位开始传输
//     out_byte_ = sb_; // 发送缓冲区的内容
//     transferring_ = true; // 开始传输状态
// }

// void SERIAL::process_transfer(EMU* emu)
// {
//     //只考虑输入的数据全为1
//     sb_ <<= 1; // 将发送缓冲区左移一位
//     ++sb_; // 模拟输入数据全为1
//     --transfer_bit_; // 减少传输位计数
//     if (transfer_bit_ < 0) {
//         transfer_bit_ = 0; 
//         end_transfer(emu); // 如果传输完成，结束传输
//     }   
// }

// void SERIAL::end_transfer(EMU* emu)
// {
//     output_buffer_.push_back(out_byte_); // 将发送的字节存入输出缓冲区
//     transferring_ = false; // 结束传输状态
//     sc_ &= 0x7F; // 清除传输中标志位
//     emu->int_flags |= INT_SERIAL; // 设置串行中断标志
// }

// u8 SERIAL::bus_read(u16 address)
// {
//     if(address == 0xFF01) {
//         // 读取发送缓冲区
//         return sb_;
//     } else if(address == 0xFF02) {
//         // 读取控制寄存器
//         return sc_;
//     }
//     std::cerr << "Invalid serial read at address: " << std::hex << address << std::endl;
//     exit(EXIT_FAILURE); // 发生错误时退出
//     return 0xFF; // 默认返回值
// }

// void SERIAL::bus_write(u16 address, u8 data)
// {
//     if(address == 0xFF01) {
//         // 写入发送缓冲区
//         sb_ = data;
//         return;
//     } else if(address == 0xFF02) {
//         // 写入控制寄存器
//         sc_ = 0x7C | (data & 0x83);
//         return;
//     } else {
//         std::cerr << "Invalid serial write at address: " << std::hex << address << std::endl;
//         exit(EXIT_FAILURE); // 发生错误时退出
//     }
// }
