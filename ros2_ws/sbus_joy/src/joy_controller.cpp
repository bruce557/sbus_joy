/**
 * joy_controller.cpp
 * 遥控器控制器实现（纯 C++，无 ROS2 依赖）
 */

#include "joy_controller.hpp"

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <utility>
#include <fcntl.h>
#include <unistd.h>

// 归一化参数
static constexpr float SBUS_CENTER = 1000.0f;
static constexpr float SBUS_RANGE  = 720.0f;

// 通道索引
static constexpr int CH5_INDEX = 4;   // CH5 在 channels[] 中的索引 (0-based)

// CH5 六档开关阈值
// 档位:  200   680   840   1160  1240  1800
// 对应:   1     2     3     4     5     6
static constexpr int CH5_THRESHOLDS[] = {440, 760, 1000, 1200, 1520};

JoyController::JoyController(const std::string& serial_port)
    : serial_port_(serial_port)
{
}

JoyController::~JoyController()
{
    close();
}

bool JoyController::open()
{
    fd_ = ::open(serial_port_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) {
        std::fprintf(stderr, "[JoyController] Failed to open serial port %s: %s\n",
                     serial_port_.c_str(), std::strerror(errno));
        return false;
    }

    if (sbus::configure_serial(fd_) != 0) {
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    // 清空旧数据
    uint8_t dummy[256];
    while (::read(fd_, dummy, sizeof(dummy)) > 0) {}

    std::printf("[JoyController] Serial port %s opened\n", serial_port_.c_str());
    return true;
}

void JoyController::close()
{
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool JoyController::get_joy_data(JoyControllerData& data)
{
    if (!read_sbus_frame()) {
        return false;
    }

    map_channels_to_joy(sbus_data_, data);

    data.frame_lost = sbus_data_.frame_lost;
    data.failsafe   = sbus_data_.failsafe;

    return true;
}

bool JoyController::read_sbus_frame()
{
    uint8_t buffer[sbus::FRAME_LENGTH];
    int bytes_read = 0;
    int timeout_count = 0;
    constexpr int MAX_TIMEOUT = 100;

    while (bytes_read < sbus::FRAME_LENGTH && timeout_count < MAX_TIMEOUT) {
        uint8_t byte;
        ssize_t n = ::read(fd_, &byte, 1);

        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(500);
                timeout_count++;
                continue;
            }
            std::fprintf(stderr, "[JoyController] Read error: %s\n", std::strerror(errno));
            return false;
        }

        if (n == 0) {
            usleep(500);
            timeout_count++;
            continue;
        }

        // 帧同步：支持 0x0F 和 0xF0 两种帧头
        if (bytes_read == 0) {
            if (byte != sbus::HEADER && byte != sbus::HEADER_ALT) {
                continue;
            }
        }

        if (bytes_read >= sbus::FRAME_LENGTH) {
            bytes_read = 0;
            continue;
        }

        buffer[bytes_read] = byte;
        bytes_read++;

        if (bytes_read == sbus::FRAME_LENGTH) {
            if (buffer[24] == sbus::FOOTER) {
                if (sbus::parse_frame(buffer, &sbus_data_) == 0) {
                    return true;
                }
            } else {
                // 帧尾错误，在帧内重新搜索帧头
                int found = 0;
                for (int i = 1; i < sbus::FRAME_LENGTH; i++) {
                    if (buffer[i] == sbus::HEADER || buffer[i] == sbus::HEADER_ALT) {
                        int remaining = sbus::FRAME_LENGTH - i;
                        std::memmove(buffer, buffer + i, remaining);
                        bytes_read = remaining;
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    bytes_read = 0;
                }
            }

            if (bytes_read == sbus::FRAME_LENGTH) {
                bytes_read = 0;
            }
        }
    }

    return false;
}

void JoyController::map_channels_to_joy(const sbus::SbusData& sbus_data, JoyControllerData& joy_data)
{
    // 归一化辅助 lambda
    auto normalize = [](uint16_t raw) -> float {
        float v = (static_cast<float>(raw) - SBUS_CENTER) / SBUS_RANGE;
        if (v >  1.0f) v =  1.0f;
        if (v < -1.0f) v = -1.0f;
        return v;
    };

    // axes[0-9]：CH1-CH4, CH6-CH11（跳过 CH5 和 CH12-CH16）
    int axe_idx = 0;
    for (int ch = 0; ch < 16; ch++) {
        if (ch == CH5_INDEX) continue;        // 跳过 CH5
        if (ch >= 11 && ch <= 15) continue;   // CH12-CH16 是按键
        joy_data.axes[axe_idx++] = normalize(sbus_data.channels[ch]);
    }

    // 交换 CH1 和 CH2：CH2→axes[0], CH1→axes[1]
    std::swap(joy_data.axes[0], joy_data.axes[1]);

    // axes[0] 反向（CH2 右摇杆）
    joy_data.axes[0] = -joy_data.axes[0] + 0.0f;

    // buttons[0] = CH5 六档开关，值 1-6
    uint16_t ch5_val = sbus_data.channels[CH5_INDEX];
    int gear = 6;
    for (int t = 0; t < 5; t++) {
        if (ch5_val < CH5_THRESHOLDS[t]) {
            gear = t + 1;
            break;
        }
    }
    joy_data.buttons[0] = gear;

    // buttons[1-5] = CH12-CH16，阈值判断：> 1000 → 1，否则 → 0
    for (int i = 0; i < 5; i++) {
        joy_data.buttons[i + 1] = (sbus_data.channels[11 + i] > static_cast<uint16_t>(SBUS_CENTER)) ? 1 : 0;
    }
}
