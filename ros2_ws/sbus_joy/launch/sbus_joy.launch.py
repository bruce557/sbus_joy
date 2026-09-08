"""
sbus_joy launch file
Launches the sbus_joy_node with configurable parameters.

Usage:
  ros2 launch sbus_joy sbus_joy.launch.py
  ros2 launch sbus_joy sbus_joy.launch.py serial_port:=/dev/ttyUSB0
  ros2 launch sbus_joy sbus_joy.launch.py publish_rate_hz:=50
"""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    # Declare launch arguments with defaults
    serial_port_arg = DeclareLaunchArgument(
        'serial_port',
        default_value='/dev/ttyUSB0',
        description='SBUS receiver serial port device path'
    )

    publish_rate_arg = DeclareLaunchArgument(
        'publish_rate_hz',
        default_value='50',
        description='Joy message publish rate in Hz'
    )

    # SBUS Joy node
    sbus_joy_node = Node(
        package='sbus_joy',
        executable='sbus_joy_node',
        name='sbus_joy_node',
        output='screen',
        parameters=[{
            'serial_port': LaunchConfiguration('serial_port'),
            'publish_rate_hz': LaunchConfiguration('publish_rate_hz'),
        }]
    )

    return LaunchDescription([
        serial_port_arg,
        publish_rate_arg,
        sbus_joy_node,
    ])
