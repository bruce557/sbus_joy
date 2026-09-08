/**
 * sbus_joy_node.cpp
 * ROS2 节点：读取 SBUS 遥控器数据，发布 sensor_msgs/Joy 消息到 /joy 话题
 * 使用 JoyController 接口获取数据，与底层驱动解耦
 *
 * 通道映射（由 JoyController 统一处理）：
 *   axes[0-9]   = CH2(反向), CH1, CH3, CH4, CH6-CH11，归一化到 [-1.0, 1.0]
 *   buttons[0]  = CH5，6档开关，值为 1-6
 *   buttons[1-5] = CH12-CH16，按键，按下=1，未按下=0
 */

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joy.hpp"
#include "joy_controller.hpp"

using namespace std::chrono_literals;

class SbusJoyNode : public rclcpp::Node
{
public:
    SbusJoyNode() : Node("sbus_joy_node")
    {
        // 声明参数
        this->declare_parameter<std::string>("serial_port", "/dev/ttyUSB0");
        this->declare_parameter<int>("publish_rate_hz", 50);

        std::string serial_port = this->get_parameter("serial_port").as_string();
        int rate = this->get_parameter("publish_rate_hz").as_int();

        // 创建发布者
        joy_pub_ = this->create_publisher<sensor_msgs::msg::Joy>("joy", 10);

        // 初始化 JoyController（纯 C++ 接口，无 ROS2 依赖）
        joy_ctrl_ = std::make_unique<JoyController>(serial_port);
        if (!joy_ctrl_->open()) {
            RCLCPP_ERROR(this->get_logger(), "Failed to open JoyController, exiting");
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
            "SBUS Joy node started, port: %s, rate: %d Hz",
            serial_port.c_str(), rate);
    }

private:
    void timer_callback()
    {
        JoyControllerData data;
        if (!joy_ctrl_->get_joy_data(data)) {
            return;
        }

        // 构建 Joy 消息
        auto joy_msg = sensor_msgs::msg::Joy();
        joy_msg.header.stamp = this->now();
        joy_msg.header.frame_id = "sbus_controller";

        // 从 JoyControllerData 填充 axes
        joy_msg.axes.resize(10);
        for (int i = 0; i < 10; i++) {
            joy_msg.axes[i] = data.axes[i];
        }

        // 从 JoyControllerData 填充 buttons
        joy_msg.buttons.resize(6);
        for (int i = 0; i < 6; i++) {
            joy_msg.buttons[i] = data.buttons[i];
        }

        joy_pub_->publish(joy_msg);

        RCLCPP_DEBUG(this->get_logger(),
            "Published Joy: axes[0]=%.2f, axes[1]=%.2f, buttons[0]=%d, buttons[1]=%d",
            joy_msg.axes[0], joy_msg.axes[1], joy_msg.buttons[0], joy_msg.buttons[1]);
    }

    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<sensor_msgs::msg::Joy>::SharedPtr joy_pub_;
    std::unique_ptr<JoyController> joy_ctrl_;
};

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<SbusJoyNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
