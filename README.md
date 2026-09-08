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
│       │   ├── sbus_serial.hpp    # SBUS 协议 C++ 头文件
│       │   └── joy_controller.hpp # 遥控器控制器接口
│       ├── launch/
│       │   └── sbus_joy.launch.py # ROS2 launch 启动文件
│       ├── service/
│       │   ├── sbus_joy.service   # systemd 服务文件
│       │   ├── install_autostart.sh   # 安装自启动脚本
│       │   └── uninstall_autostart.sh # 卸载自启动脚本
│       ├── src/
│       │   ├── sbus_serial.cpp        # SBUS 串口配置与帧解析
│       │   ├── sbus_joy_node.cpp      # ROS2 节点主程序
│       │   ├── joy_controller.cpp     # 遥控器控制器实现
│       │   └── test_joy_controller.cpp # 独立测试程序
│       ├── CMakeLists.txt
│       └── package.xml
└── README.md
```

## 通道映射

SBUS 共 16 个通道，按功能分为三组：

| SBUS 通道 | Joy 字段 | 类型 | 说明 |
|-----------|----------|------|------|
| CH1–CH4, CH6–CH11 | `axes[0–9]` | 摇杆/拨杆 | 线性归一化到 `[-1.0, 1.0]`，中值 1000 对应 0.0 |
| CH5 | `buttons[0]` | 6档开关 | 值为 1–6，对应 SBUS 原始值 200/680/840/1160/1240/1800 |
| CH12–CH16 | `buttons[1–5]` | 按键 | 二值化：原始值 > 1000 → `1`（按下），否则 → `0` |

CH5 六档开关映射：

| SBUS 原始值 | buttons[0] 输出 |
|-------------|----------------|
| ~200 | 1 |
| ~680 | 2 |
| ~840 | 3 |
| ~1160 | 4 |
| ~1240 | 5 |
| ~1800 | 6 |

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

### 方式一：使用 launch（推荐）

```bash
# 加载环境
source /opt/ros/humble/setup.bash
source ~/ros2_ws/install/setup.bash
export ROS_LOG_DIR=/root/.ros/log

# 默认参数启动（串口 /dev/ttyACM1，频率 50Hz）
ros2 launch sbus_joy sbus_joy.launch.py

# 指定串口
ros2 launch sbus_joy sbus_joy.launch.py serial_port:=/dev/ttyACM0

# 指定发布频率
ros2 launch sbus_joy sbus_joy.launch.py publish_rate_hz:=100

# 同时指定多个参数
ros2 launch sbus_joy sbus_joy.launch.py serial_port:=/dev/ttyACM0 publish_rate_hz:=100
```

### 方式二：使用 ros2 run

```bash
# 加载环境
source /opt/ros/humble/setup.bash
source ~/ros2_ws/install/setup.bash
export ROS_LOG_DIR=/root/.ros/log

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
export ROS_LOG_DIR=/root/.ros/log

ros2 topic echo /joy
```

正常输出示例（摇杆中位，CH5 在档位 3，按键未按下）：

```yaml
---
header:
  stamp:
    sec: 1787638816
    nanosec: 955476194
  frame_id: sbus_controller
axes:
- 0.0      # CH2  右摇杆
- 0.0      # CH1  右摇杆
- 0.0      # CH3  左拨杆
- 0.0      # CH4  左摇杆
- 0.0      # CH6  左上拨杆
- 0.0      # CH7  右上拨杆
- 0.0      # CH8  副摇杆
- 0.0      # CH9  副摇杆
- 0.0      # CH10 左上按键,值为-1，0，1
- 0.0      # CH11 右上按键,值为-1，0，1
buttons:
- 3        # CH5 六档开关（当前档位 3）
- 0        # CH12 L1按键（未按下）
- 0        # CH13 L2按键（未按下)
- 0        # CH14 L3按键（未按下)
- 0        # CH15 L4按键（未按下)
- 0        # CH16 L5按键（未按下)
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

## 开机自启动

使用 systemd service 实现开机自动启动 sbus_joy 节点。

### 安装自启动

```bash
# 进入 service 目录
cd ~/ros2_ws/sbus_joy/service

# 一键安装（需要 sudo 权限）
sudo bash install_autostart.sh
```

安装完成后，系统启动时会自动运行 sbus_joy 节点。

### 卸载自启动

```bash
cd ~/ros2_ws/sbus_joy/service
sudo bash uninstall_autostart.sh
```

### 手动管理服务

```bash
# 查看服务状态
systemctl status sbus_joy

# 启动服务
sudo systemctl start sbus_joy

# 停止服务
sudo systemctl stop sbus_joy

# 重启服务
sudo systemctl restart sbus_joy

# 查看实时日志
journalctl -u sbus_joy -f
```

### 修改自启动参数

编辑 `/etc/systemd/system/sbus_joy.service` 文件，修改 `ExecStart` 行：

```bash
# 修改串口
ExecStart=/bin/bash -c '... ros2 launch sbus_joy sbus_joy.launch.py serial_port:=/dev/ttyACM0 ...'

# 修改发布频率
ExecStart=/bin/bash -c '... ros2 launch sbus_joy sbus_joy.launch.py publish_rate_hz:=100 ...'
```

修改后执行：
```bash
sudo systemctl daemon-reload
sudo systemctl restart sbus_joy
```

---

## 常见问题

**Q: 启动报 `打开串口失败`**
A: 检查串口设备路径是否正确（`ls /dev/ttyACM*`），以及当前用户是否有串口权限（`sudo usermod -aG dialout $USER`）。

**Q: 数据一直为 0 或无输出**
A: 确认 SBUS 接收器已正确连接并上电，串口线序正确。可尝试降低 `publish_rate_hz` 或检查 `dmesg | grep tty` 查看串口状态。
