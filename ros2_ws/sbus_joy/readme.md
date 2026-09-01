# SBUS 遥控器 ROS2 节点

读取 SBUS 遥控器数据，发布 `sensor_msgs/Joy` 消息到 `/joy` 话题。

## 通道映射

| SBUS 通道 | Joy 字段 | 说明 |
|-----------|----------|------|
| CH1-CH11 | `axes[0-10]` | 摇杆/拨杆，归一化到 [-1.0, 1.0] |
| CH12-CH16 | `buttons[0-4]` | 按键，按下=1，未按下=0 |

## 编译

```bash
source /opt/ros/humble/setup.bash
cd ros2_ws
colcon build --packages-select sbus_joy --cmake-clean-cache
```

## 运行

```bash
source /opt/ros/humble/setup.bash
source ros2_ws/install/setup.bash
export ROS_LOG_DIR=/root/.ros/log

ros2 run sbus_joy sbus_joy_node --ros-args -p serial_port:=/dev/ttyACM1
```

### 参数

| 参数名 | 默认值 | 说明 |
|--------|--------|------|
| `serial_port` | `/dev/ttyACM1` | 串口设备路径 |
| `publish_rate_hz` | `50` | 发布频率 (Hz) |

## 验证数据

```bash
source /opt/ros/humble/setup.bash
source ros2_ws/install/setup.bash
export ROS_LOG_DIR=/root/.ros/log
ros2 topic echo /joy
```


