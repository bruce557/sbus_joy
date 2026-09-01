/**
 * sbus_joy_node.cpp
 * ROS2 节点：读取 SBUS 遥控器数据，发布 sensor_msgs/Joy 消息到 /joy 话题
 *
 * 通道映射：
 *   axes[0-10]  = CH1-CH11，摇杆/拨杆，归一化到 [-1.0, 1.0]
 *   buttons[0-4] = CH12-CH16，按键，按下=1，未按下=0
 */

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joy.hpp"
#include "sbus_serial.hpp"

using namespace std::chrono_literals;

// SBUS 通道值范围（用于归一化）
static constexpr float SBUS_MIN    = 200.0f;
static constexpr float SBUS_MAX    = 1800.0f;
static constexpr float SBUS_CENTER = 1000.0f;
static constexpr float SBUS_RANGE  = 1000.0f;   // (SBUS_MAX - SBUS_MIN) / 2

// 通道分配：CH1-CH11 → axes，CH12-CH16 → buttons
static constexpr int AXE_COUNT    = 11;   // 摇杆/拨杆通道数
static constexpr int BUTTON_COUNT = 5;    // 按键通道数

class SbusJoyNode : public rclcpp::Node
{
public:
    SbusJoyNode() : Node("sbus_joy_node")
    {
        // 声明参数
        this->declare_parameter<std::string>("serial_port", "/dev/ttyACM1");
        this->declare_parameter<int>("publish_rate_hz", 50);

        serial_port_ = this->get_parameter("serial_port").as_string();
        int rate = this->get_parameter("publish_rate_hz").as_int();

        // 创建发布者
        joy_pub_ = this->create_publisher<sensor_msgs::msg::Joy>("joy", 10);

        // 打开串口
        if (!open_serial()) {
            RCLCPP_ERROR(this->get_logger(), "串口打开失败，退出");
            rclcpp::shutdown();
            return;
        }

        // 创建定时器，按指定频率发布
        auto period = std::chrono::milliseconds(1000 / rate);
        timer_ = this->create_wall_timer(
            period,
            std::bind(&SbusJoyNode::timer_callback, this)
        );

        RCLCPP_INFO(this->get_logger(),
            "SBUS Joy 节点启动，串口: %s, 发布频率: %d Hz",
            serial_port_.c_str(), rate);
    }

    ~SbusJoyNode()
    {
        if (fd_ >= 0) {
            close(fd_);
        }
    }

private:
    bool open_serial()
    {
        fd_ = open(serial_port_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd_ < 0) {
            RCLCPP_ERROR(this->get_logger(), "打开串口 %s 失败: %s",
                serial_port_.c_str(), std::strerror(errno));
            return false;
        }

        if (sbus::configure_serial(fd_) != 0) {
            close(fd_);
            fd_ = -1;
            return false;
        }

        // 清空旧数据
        uint8_t dummy[256];
        while (read(fd_, dummy, sizeof(dummy)) > 0) {}

        RCLCPP_INFO(this->get_logger(), "串口 %s 打开成功", serial_port_.c_str());
        return true;
    }

    void timer_callback()
    {
        if (!read_sbus_frame()) {
            return;
        }

        // 构建 Joy 消息
        auto joy_msg = sensor_msgs::msg::Joy();
        joy_msg.header.stamp = this->now();
        joy_msg.header.frame_id = "sbus_controller";

        // CH1-CH11 映射到 axes，归一化到 [-1.0, 1.0]
        joy_msg.axes.resize(AXE_COUNT);
        for (int i = 0; i < AXE_COUNT; i++) {
            float normalized = (static_cast<float>(sbus_data_.channels[i]) - SBUS_CENTER) / SBUS_RANGE;
            if (normalized >  1.0f) normalized =  1.0f;
            if (normalized < -1.0f) normalized = -1.0f;
            joy_msg.axes[i] = normalized;
        }

        // CH12-CH16 映射到 buttons，阈值判断：归一化值 > 0 → 按下(1)，否则 → 未按下(0)
        joy_msg.buttons.resize(BUTTON_COUNT);
        for (int i = 0; i < BUTTON_COUNT; i++) {
            float normalized = (static_cast<float>(sbus_data_.channels[AXE_COUNT + i]) - SBUS_CENTER) / SBUS_RANGE;
            joy_msg.buttons[i] = (normalized > 0.0f) ? 1 : 0;
        }

        joy_pub_->publish(joy_msg);

        RCLCPP_DEBUG(this->get_logger(),
            "发布 Joy: axes[0]=%.2f, axes[1]=%.2f, buttons[0]=%d, buttons[1]=%d",
            joy_msg.axes[0], joy_msg.axes[1], joy_msg.buttons[0], joy_msg.buttons[1]);
    }

    bool read_sbus_frame()
    {
        uint8_t buffer[sbus::FRAME_LENGTH];
        int bytes_read = 0;
        int timeout_count = 0;
        constexpr int MAX_TIMEOUT = 100;

        while (bytes_read < sbus::FRAME_LENGTH && timeout_count < MAX_TIMEOUT) {
            uint8_t byte;
            ssize_t n = read(fd_, &byte, 1);

            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    usleep(500);
                    timeout_count++;
                    continue;
                }
                RCLCPP_ERROR(this->get_logger(), "读取错误: %s", std::strerror(errno));
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

        if (timeout_count >= MAX_TIMEOUT) {
            RCLCPP_WARN(this->get_logger(), "读取 SBUS 帧超时");
        }

        return false;
    }

    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<sensor_msgs::msg::Joy>::SharedPtr joy_pub_;

    std::string serial_port_;
    int fd_ = -1;
    sbus::SbusData sbus_data_;
};

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<SbusJoyNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
