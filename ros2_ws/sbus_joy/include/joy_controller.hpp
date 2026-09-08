/**
 * joy_controller.hpp
 * 遥控器数据接口（纯 C++，无 ROS2 依赖）
 *
 * 上层调用示例：
 *   JoyController joy("/dev/ttyUSB0");
 *   if (joy.open() && joy.get_joy_data(data)) {
 *       // 使用 data.axes[0-9], data.buttons[0-5]
 *   }
 */

#ifndef JOY_CONTROLLER_HPP
#define JOY_CONTROLLER_HPP

#include <cstdint>
#include <string>
#include "sbus_serial.hpp"

// 遥控器数据结构
struct JoyControllerData {
    // 10 个轴：归一化到 [-1.0, 1.0]
    // axes[0] = CH2 右摇杆（反向）
    // axes[1] = CH1 右摇杆
    // axes[2] = CH3 左拨杆
    // axes[3] = CH4 左摇杆
    // axes[4] = CH6  左上拨杆
    // axes[5] = CH7  右上拨杆
    // axes[6] = CH8  副摇杆
    // axes[7] = CH9  副摇杆
    // axes[8] = CH10 左上按键 (-1/0/1)
    // axes[9] = CH11 右上按键 (-1/0/1)
    float axes[10] = {};

    // 6 个按键
    // buttons[0] = CH5 六档开关 (1-6)
    // buttons[1] = CH12 L1 按键 (0/1)
    // buttons[2] = CH13 L2 按键 (0/1)
    // buttons[3] = CH14 L3 按键 (0/1)
    // buttons[4] = CH15 L4 按键 (0/1)
    // buttons[5] = CH16 L5 按键 (0/1)
    int buttons[6] = {};

    // 状态标志
    bool frame_lost = false;
    bool failsafe   = false;
};

/**
 * 遥控器控制器类
 * 封装串口操作、SBUS 帧读取、通道映射逻辑
 */
class JoyController
{
public:
    /**
     * 构造函数
     * @param serial_port 串口设备路径，默认 /dev/ttyUSB0
     */
    explicit JoyController(const std::string& serial_port = "/dev/ttyUSB0");

    ~JoyController();

    // 禁止拷贝
    JoyController(const JoyController&) = delete;
    JoyController& operator=(const JoyController&) = delete;

    /**
     * 打开并配置串口
     * @return true 成功, false 失败
     */
    bool open();

    /**
     * 关闭串口
     */
    void close();

    /**
     * 读取一帧 SBUS 数据并转换为 JoyControllerData
     * 非阻塞，若超时未读到完整帧则返回 false
     * @param data 输出数据
     * @return true 成功获取一帧, false 超时或错误
     */
    bool get_joy_data(JoyControllerData& data);

    /**
     * 获取串口文件描述符（供外部高级用法）
     */
    int fd() const { return fd_; }

    /**
     * 串口是否已打开
     */
    bool is_open() const { return fd_ >= 0; }

private:
    // 读取并解析一帧 SBUS 原始数据
    bool read_sbus_frame();

    // 将原始 SBUS 通道数据映射为 JoyControllerData
    void map_channels_to_joy(const sbus::SbusData& sbus_data, JoyControllerData& joy_data);

    std::string serial_port_;
    int fd_ = -1;
    sbus::SbusData sbus_data_;
};

#endif  // JOY_CONTROLLER_HPP
