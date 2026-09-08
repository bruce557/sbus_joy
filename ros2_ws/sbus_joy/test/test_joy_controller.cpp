/**
 * test_joy_controller.cpp
 * 独立测试程序：直接调用 JoyController 接口读取遥控器数据
 * 
 * 编译（不依赖 ROS2）：
 *   g++ -std=c++11 -o test_joy test_joy_controller.cpp \
 *       joy_controller.cpp sbus_serial.cpp -I../include
 * 
 * 运行：
 *   ./test_joy [/dev/ttyUSB0]
 */

#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <unistd.h>
#include "joy_controller.hpp"

static volatile bool g_running = true;

void signal_handler(int) {
    g_running = false;
}

void print_joy_data(const JoyControllerData& data)
{
    std::printf("\r\033[K");  // 清除当前行
    std::printf("[Joy] ");

    // 打印 axes
    for (int i = 0; i < 10; i++) {
        std::printf("A%d=%+6.2f ", i, data.axes[i]);
    }

    // 打印 buttons
    std::printf("| ");
    for (int i = 0; i < 6; i++) {
        std::printf("B%d=%d ", i, data.buttons[i]);
    }

    // 状态标志
    if (data.failsafe)   std::printf("| FAILSAFE");
    if (data.frame_lost) std::printf("| LOST");

    std::fflush(stdout);
}

int main(int argc, char* argv[])
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    const char* port = "/dev/ttyUSB0";
    if (argc >= 2) {
        port = argv[1];
    }

    std::printf("========================================\n");
    std::printf("JoyController Test\n");
    std::printf("Port: %s\n", port);
    std::printf("Press Ctrl+C to exit\n");
    std::printf("========================================\n\n");

    JoyController joy(port);

    if (!joy.open()) {
        std::fprintf(stderr, "Failed to open JoyController on %s\n", port);
        return 1;
    }

    JoyControllerData data;
    int frame_count = 0;

    std::printf("Reading data...\n");

    while (g_running) {
        if (joy.get_joy_data(data)) {
            frame_count++;
            print_joy_data(data);

            // 每 100 帧打印一次详细信息
            if (frame_count % 100 == 0) {
                std::printf("\n--- Frame #%d ---\n", frame_count);
                std::printf("  axes:    ");
                for (int i = 0; i < 10; i++) {
                    std::printf("[%d]=%+.3f ", i, data.axes[i]);
                }
                std::printf("\n");
                std::printf("  buttons: ");
                for (int i = 0; i < 6; i++) {
                    std::printf("[%d]=%d ", i, data.buttons[i]);
                }
                std::printf("\n");
            }
        } else {
            usleep(1000);
        }
    }

    std::printf("\n\nExit. Total frames: %d\n", frame_count);
    return 0;
}
