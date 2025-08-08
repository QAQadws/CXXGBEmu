#include "serial.h"
#include "emu.h"
#include<iostream>
void SERIAL::init()
{
    sb_ = 0xFF; 
    sc_ = 0x7C; // 0x7C = 0111 1100
    transferring_ = false;
}

void SERIAL::tick(EMU* emu)
{
    if(!transferring_ && is_master() && transfer_enable()){
        begin_transfer(); // 如果是主机并且传输使能，开始传
    }
    else if(transferring_){
        process_transfer(emu); // 如果正在传输，处理传输
    }
}

void SERIAL::begin_transfer()
{
    transfer_bit_ = 7; // 从最高位开始传输
    out_byte_ = sb_; // 发送缓冲区的内容
    transferring_ = true; // 开始传输状态
}

void SERIAL::process_transfer(EMU* emu)
{
    //只考虑输入的数据全为1
    sb_ <<= 1; // 将发送缓冲区左移一位
    ++sb_; // 模拟输入数据全为1
    --transfer_bit_; // 减少传输位计数
    if (transfer_bit_ < 0) {
        transfer_bit_ = 0; 
        end_transfer(emu); // 如果传输完成，结束传输
    }   
}

void SERIAL::end_transfer(EMU* emu)
{
    output_buffer_.push_back(out_byte_); // 将发送的字节存入输出缓冲区
    transferring_ = false; // 结束传输状态
    sc_ &= 0x7F; // 清除传输中标志位
    emu->int_flags |= INT_SERIAL; // 设置串行中断标志
}

u8 SERIAL::bus_read(u16 address)
{
    if(address == 0xFF01) {
        // 读取发送缓冲区
        return sb_;
    } else if(address == 0xFF02) {
        // 读取控制寄存器
        return sc_;
    }
    std::cerr << "Invalid serial read at address: " << std::hex << address << std::endl;
    exit(EXIT_FAILURE); // 发生错误时退出
    return 0xFF; // 默认返回值
}

void SERIAL::bus_write(u16 address, u8 data)
{
    if(address == 0xFF01) {
        // 写入发送缓冲区
        sb_ = data;
        return;
    } else if(address == 0xFF02) {
        // 写入控制寄存器
        sc_ = 0x7C | (data & 0x83);
        return;
    } else {
        std::cerr << "Invalid serial write at address: " << std::hex << address << std::endl;
        exit(EXIT_FAILURE); // 发生错误时退出
    }
}
