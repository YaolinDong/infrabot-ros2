[English](README.md) · 简体中文

# InfraBot ROS 2

InfraBot H7C 七轴机械臂的 ROS 2 Jazzy 驱动：基于 [InfraBot SDK](https://github.com/YaolinDong/infrabot-sdk) 的 ros2_control 硬件接口，以及 launch 文件和控制器配置。

| 功能包 | 内容 |
|---|---|
| `infrabot_hardware` | `infrabot_hardware/InfrabotSystem`：J1–J7 位置指令；位置、速度、力矩状态 |
| `infrabot_bringup` | 带 ros2_control 的 URDF、控制器配置、`infrabot.launch.py` |

机械臂模型来自 [infrabot_description](https://github.com/YaolinDong/infrabot-description)。

## 编译

```bash
sudo apt install python3-vcstool python3-colcon-common-extensions python3-rosdep libssl-dev
mkdir -p ~/infrabot_ws/src && cd ~/infrabot_ws/src
git clone https://github.com/YaolinDong/infrabot-ros2.git
vcs import . < infrabot-ros2/infrabot.repos
cd ~/infrabot_ws
rosdep install --from-paths src --ignore-src -y --skip-keys infrabot_sdk
colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release -DINFRABOT_BUILD_GUI=OFF
source install/setup.bash
```

请在 conda 环境之外编译，让 colcon 使用系统 Python。

## 运行

不接硬件：

```bash
ros2 launch infrabot_bringup infrabot.launch.py use_mock_hardware:=true
```

连接机械臂：

```bash
ros2 launch infrabot_bringup infrabot.launch.py robot_ip:=192.168.0.50
```

用轨迹控制器运动：

```bash
ros2 action send_goal /joint_trajectory_controller/follow_joint_trajectory control_msgs/action/FollowJointTrajectory \
  "{trajectory: {joint_names: [Joint_1, Joint_2, Joint_3, Joint_4, Joint_5, Joint_6, Joint_7],
    points: [{positions: [0.0, 0.5, 0.0, 0.8, 0.0, 0.3, 0.0], time_from_start: {sec: 5}}]}}"
```

加 `robot_controller:=forward_position_controller` 可改用位置直通控制器。

## 在机械臂上的行为

| 硬件状态 | 驱动做什么 |
|---|---|
| configure | 连接控制器，等待遥测，选择 MIT 驱动模式 |
| activate | **使能 J1–J7**，进入 RT_JOINT，从当前保持位置开始发指令 |
| deactivate、出错 | 受控停止，机械臂保持使能并停在原地 |
| cleanup、shutdown | 断开连接，控制器继续保持机械臂 |

驱动不会失能驱动器。指令流中断 100 ms 或遥测过期时，控制器会让机械臂停下。重力补偿、阻抗、导纳和夹爪目前还没有接入。

> [!WARNING]
> 启动 launch 文件会使能机械臂。请先清空工作范围，并接好独立的硬件急停。请阅读 [SAFETY.md](https://github.com/YaolinDong/infrabot/blob/main/SAFETY.md)。

## 版本兼容

固件 0.1.1、SDK 0.1.3，协议 1.0；ROS 2 Jazzy，Ubuntu 24.04。

[Apache-2.0](LICENSE) · [更新记录](CHANGELOG.md) · [参与贡献](CONTRIBUTING.md)

Copyright © 2026 YaolinDong（董耀琳）
