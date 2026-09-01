/**
 * sbus_serial.cpp
 * SBUS 串口配置与帧解析 C++ 实现
 * 实际配置: 115200, 8N1
 */

#include "sbus_serial.hpp"

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/ioctl.h>
#include <asm/termbits.h>

namespace sbus {

int configure_serial(int fd)
{
    struct termios2 tty{};
    int ret = ioctl(fd, TCGETS2, &tty);
    if (ret != 0) {
        std::perror("[sbus] TCGETS2 failed");
        return -1;
    }

    std::printf("[sbus] termios2 acquired\n");

    // 清除旧配置
    tty.c_cflag &= ~(CBAUD | CSIZE | PARENB | PARODD | CSTOPB | CRTSCTS | HUPCL);
    tty.c_cflag |= CREAD | CLOCAL;

    // 自定义波特数 115200
    tty.c_cflag |= BOTHER;
    tty.c_ispeed = 115200;
    tty.c_ospeed = 115200;

    // 8N1
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~PARODD;
    tty.c_cflag &= ~CRTSCTS;

    // 原始输入
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP |
                     INLCR | IGNCR | ICRNL | IXON | IXOFF | IXANY);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;

    ret = ioctl(fd, TCSETS2, &tty);
    if (ret != 0) {
        std::perror("[sbus] TCSETS2 failed");
        return -1;
    }

    ioctl(fd, TCFLSH, TCIOFLUSH);

    std::printf("[sbus] Serial configured: 115200, 8N1\n");
    return 0;
}

int parse_frame(const uint8_t* frame, SbusData* data)
{
    if (!frame || !data) return -1;

    if (frame[0] != HEADER || frame[24] != FOOTER) {
        return -1;
    }

    *data = SbusData{};

    uint32_t buffer = 0;
    int bit_pos  = 0;
    int byte_idx = 1;

    for (int ch = 0; ch < CHANNEL_COUNT; ch++) {
        while (bit_pos < 11 && byte_idx < 24) {
            buffer |= static_cast<uint32_t>(frame[byte_idx]) << bit_pos;
            bit_pos  += 8;
            byte_idx++;
        }
        data->channels[ch] = buffer & 0x07FF;
        buffer  >>= 11;
        bit_pos  -= 11;
    }

    // 解析标志位字节 (byte 23)
    uint8_t flags = frame[23];
    data->channel17  = (flags >> 7) & 0x01;
    data->channel18  = (flags >> 6) & 0x01;
    data->frame_lost = (flags >> 5) & 0x01;
    data->failsafe   = (flags >> 4) & 0x01;

    return 0;
}

void print_data(const SbusData& data)
{
    static int frame_count = 0;
    frame_count++;

    std::printf("\r[Frame #%04d] ", frame_count);
    for (int i = 0; i < CHANNEL_COUNT; i++) {
        std::printf("CH%02d=%4d ", i + 1, data.channels[i]);
    }
    std::printf("| lost:%d failsafe:%d  ", data.frame_lost, data.failsafe);
    std::fflush(stdout);
}

void print_raw(const uint8_t* frame, int len)
{
    std::printf("Raw data: ");
    for (int i = 0; i < len && i < 50; i++) {
        std::printf("%02X ", frame[i]);
    }
    std::printf("\n");
}

}  // namespace sbus
