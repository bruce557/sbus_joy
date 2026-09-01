# SBUS 遥控器 ROS2 节点

将 SBUS 遥控器数据通过 ROS2 发布为 `sensor_msgs/msg/Joy` 消息，供下游节点使用。

适用于 RK3588 (ARM64) + ROS2 Humble 环境。

---

## 项目结构

```
Controller/
├── ros2_ws/
│   └── sbus_joy/              # ROS2 功能包
│       ├── include/
│       │   └── sbus_serial.hpp    # SBUS 协议 C++ 头文件
│       ├── src/
│       │   ├── sbus_serial.cpp    # SBUS 串口配置与帧解析
│       │   └── sbus_joy_node.cpp  # ROS2 节点主程序
│       ├── CMakeLists.txt
│       └── package.xml
└── README.md
```

## 通道映射

SBUS 共 16 个通道，按功能分为两组：

| SBUS 通道 | Joy 字段 | 类型 | 说明 |
|-----------|----------|------|------|
| CH1–CH11 | `axes[0–10]` | 摇杆/拨杆 | 线性归一化到 `[-1.0, 1.0]`，中值 1000 对应 0.0 |
| CH12–CH16 | `buttons[0–4]` | 按键 | 二值化：原始值 > 1000 → `1`（按下），否则 → `0` |

归一化参数：

| 参数 | 值 | 说明 |
|------|----|------|
| SBUS 中值 | 1000 | 摇杆中位 / 按键切换阈值 |
| SBUS 行程 | 1000 | 从中位到极值的范围 |

---

## 编译

> **注意：** 必须在 RK3588 目标板上编译，不可在 x86 开发机上编译后拷贝，否则会报 `Exec format error`。

```bash
# 1. 加载 ROS2 环境
source /opt/ros/humble/setup.bash

# 2. 进入工作空间
cd ~/ros2_ws

# 3. 编译（首次或代码修改后）
colcon build --packages-select sbus_joy --cmake-clean-cache
```

编译成功后，可执行文件位于：
```
ros2_ws/install/sbus_joy/lib/sbus_joy/sbus_joy_node
```
---

## 运行

```bash
# 加载环境
source /opt/ros/humble/setup.bash
source ~/ros2_ws/install/setup.bash

# 启动节点（默认串口 /dev/ttyACM1，默认频率 50Hz）
ros2 run sbus_joy sbus_joy_node

# 指定串口和发布频率
ros2 run sbus_joy sbus_joy_node --ros-args \
  -p serial_port:=/dev/ttyACM0 \
  -p publish_rate_hz:=50
```

### 参数说明

| 参数名 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `serial_port` | string | `/dev/ttyACM1` | SBUS 接收器串口设备路径 |
| `publish_rate_hz` | int | `50` | Joy 消息发布频率（Hz） |

运行时动态修改参数（无需重启节点）：
```bash
ros2 param set /sbus_joy_node publish_rate_hz 50
```

> 注：`serial_port` 仅在启动时生效，运行时修改无效。

---

## 验证

### 1. 查看实时数据

另开一个终端：

```bash
source /opt/ros/humble/setup.bash
source ~/ros2_ws/install/setup.bash
ros2 topic echo /joy
```

正常输出示例（摇杆中位，按键未按下）：

```yaml
---
header:
  stamp:
    sec: 1787638816
    nanosec: 955476194
  frame_id: sbus_controller
axes:
- 0.0      # CH1  
- 0.0      # CH2  
- 0.0      # CH3  
- 0.0      # CH4  
- 0.5      # CH5  
- 0.0      # CH6
- 0.0      # CH7
- 0.0      # CH8
- 0.0      # CH9
- 0.0      # CH10
- 0.0      # CH11
buttons:
- 0        # CH12 
- 0        # CH13
- 0        # CH14
- 0        # CH15
- 0        # CH16
```

### 2. 检查话题发布频率

```bash
ros2 topic hz /joy
```

应接近设定的 `publish_rate_hz`（如 50 Hz）。

### 3. 检查节点状态

```bash
ros2 node info /sbus_joy_node
ros2 param list /sbus_joy_node
```

---

## 常见问题

**Q: 启动报 `打开串口失败`**
A: 检查串口设备路径是否正确（`ls /dev/ttyACM*`），以及当前用户是否有串口权限（`sudo usermod -aG dialout $USER`）。

**Q: 数据一直为 0 或无输出**
A: 确认 SBUS 接收器已正确连接并上电，串口线序正确。可尝试降低 `publish_rate_hz` 或检查 `dmesg | grep tty` 查看串口状态。
