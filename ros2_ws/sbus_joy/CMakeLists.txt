cmake_minimum_required(VERSION 3.8)
project(sbus_joy)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)

# SBUS C++ 底层驱动（静态库）
add_library(sbus_serial STATIC
  src/sbus_serial.cpp
)
target_include_directories(sbus_serial PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# JoyController 控制器库（纯 C++，无 ROS2 依赖）
add_library(joy_controller STATIC
  src/joy_controller.cpp
)
target_include_directories(joy_controller PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(joy_controller sbus_serial)

# ROS2 节点
add_executable(sbus_joy_node src/sbus_joy_node.cpp)
target_include_directories(sbus_joy_node PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
ament_target_dependencies(sbus_joy_node rclcpp sensor_msgs)
target_link_libraries(sbus_joy_node joy_controller)

# 独立测试程序（无 ROS2 依赖，可单独编译运行）
add_executable(test_joy_controller src/test_joy_controller.cpp)
target_include_directories(test_joy_controller PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(test_joy_controller joy_controller)

install(TARGETS sbus_joy_node test_joy_controller
  DESTINATION lib/${PROJECT_NAME}
)

# Install launch files
install(DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}/launch
)

ament_package()
