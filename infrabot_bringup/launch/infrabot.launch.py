# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Yaolin Dong
"""Start the H7C ros2_control stack: real hardware or mock components."""
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import Command, FindExecutable, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    robot_ip = LaunchConfiguration('robot_ip')
    use_mock_hardware = LaunchConfiguration('use_mock_hardware')
    robot_controller = LaunchConfiguration('robot_controller')
    launch_rviz = LaunchConfiguration('launch_rviz')

    description = ParameterValue(Command([
        FindExecutable(name='xacro'), ' ',
        PathJoinSubstitution([FindPackageShare('infrabot_bringup'), 'urdf', 'infrabot_h7c.urdf.xacro']),
        ' robot_ip:=', robot_ip, ' use_mock_hardware:=', use_mock_hardware,
    ]), value_type=str)
    controllers = PathJoinSubstitution([FindPackageShare('infrabot_bringup'), 'config',
                                        'infrabot_controllers.yaml'])
    rviz_config = PathJoinSubstitution([FindPackageShare('infrabot_description'), 'rviz',
                                        'infrabot.rviz'])

    return LaunchDescription([
        DeclareLaunchArgument('robot_ip', default_value='192.168.0.50',
                              description='IP address of the H7C controller'),
        DeclareLaunchArgument('use_mock_hardware', default_value='false',
                              description='Use mock_components instead of the arm'),
        DeclareLaunchArgument('robot_controller', default_value='joint_trajectory_controller',
                              choices=['joint_trajectory_controller', 'forward_position_controller'],
                              description='Arm controller to start'),
        DeclareLaunchArgument('launch_rviz', default_value='true', description='Start RViz'),
        Node(package='robot_state_publisher', executable='robot_state_publisher',
             parameters=[{'robot_description': description}], output='screen'),
        Node(package='controller_manager', executable='ros2_control_node',
             parameters=[controllers], output='screen',
             remappings=[('~/robot_description', '/robot_description')]),
        Node(package='controller_manager', executable='spawner',
             arguments=['joint_state_broadcaster', '--controller-manager', '/controller_manager']),
        Node(package='controller_manager', executable='spawner',
             arguments=[robot_controller, '--controller-manager', '/controller_manager']),
        Node(package='rviz2', executable='rviz2', arguments=['-d', rviz_config],
             condition=IfCondition(launch_rviz), output='log'),
    ])
