/**
 * sbus_serial.hpp
 * SBUS 串口操作 C++ 头文件
 * 实际配置: 115200, 8N1
 */

#ifndef SBUS_SERIAL_HPP
#define SBUS_SERIAL_HPP

#include <cstdint>
#include <string>

namespace sbus {

// SBUS 协议常量
constexpr int FRAME_LENGTH   = 25;
constexpr uint8_t HEADER     = 0x0F;
constexpr uint8_t HEADER_ALT = 0xF0;  // 无反相USB转接板兼容
constexpr uint8_t FOOTER     = 0x00;
constexpr int CHANNEL_COUNT  = 16;

// SBUS 数据结构
struct SbusData {
    uint16_t channels[CHANNEL_COUNT]{};
    uint8_t  channel17   = 0;
    uint8_t  channel18   = 0;
    uint8_t  frame_lost  = 0;
    uint8_t  failsafe    = 0;
};

/**
 * 配置串口为 SBUS 通信参数 (115200, 8N1)
 * @param fd 已打开的串口文件描述符
 * @return 0 成功, -1 失败
 */
int configure_serial(int fd);

/**
 * 解析一帧 SBUS 数据 (25 字节)
 * @param frame 指向 25 字节帧数据的指针
 * @param data  输出解析后的通道数据
 * @return 0 成功, -1 帧头/帧尾校验失败
 */
int parse_frame(const uint8_t* frame, SbusData* data);

/**
 * 打印 SBUS 通道数据（调试用）
 */
void print_data(const SbusData& data);

/**
 * 打印原始帧数据（调试用）
 */
void print_raw(const uint8_t* frame, int len);

}  // namespace sbus

#endif  // SBUS_SERIAL_HPP
