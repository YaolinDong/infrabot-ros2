English · [简体中文](README.zh-CN.md)

# InfraBot ROS 2

ROS 2 Jazzy driver for the InfraBot H7C seven-axis arm. It is a ros2_control hardware interface built on the [InfraBot SDK](https://github.com/YaolinDong/infrabot-sdk), plus launch files and controller configuration.

| Package | Contents |
|---|---|
| `infrabot_hardware` | `infrabot_hardware/InfrabotSystem`: J1–J7 position commands; position, velocity and effort states |
| `infrabot_bringup` | URDF with ros2_control, controller configuration, `infrabot.launch.py` |

The robot model comes from [infrabot_description](https://github.com/YaolinDong/infrabot-description).

## Build

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

Build outside a conda environment so that colcon uses the system Python.

## Run

Without hardware:

```bash
ros2 launch infrabot_bringup infrabot.launch.py use_mock_hardware:=true
```

With the arm:

```bash
ros2 launch infrabot_bringup infrabot.launch.py robot_ip:=192.168.0.50
```

Move with the trajectory controller:

```bash
ros2 action send_goal /joint_trajectory_controller/follow_joint_trajectory control_msgs/action/FollowJointTrajectory \
  "{trajectory: {joint_names: [Joint_1, Joint_2, Joint_3, Joint_4, Joint_5, Joint_6, Joint_7],
    points: [{positions: [0.0, 0.5, 0.0, 0.8, 0.0, 0.3, 0.0], time_from_start: {sec: 5}}]}}"
```

`robot_controller:=forward_position_controller` starts a forward position controller instead.

## Behaviour on the arm

| Hardware state | What the driver does |
|---|---|
| configure | Connects, waits for telemetry and selects the MIT drive profile |
| activate | **Enables J1–J7**, enters RT_JOINT and streams from the current hold position |
| deactivate, error | Controlled stop. The arm stays enabled and holds |
| cleanup, shutdown | Disconnects. The controller keeps holding the arm |

The driver never disables the drives. The controller stops the arm if the stream stops for 100 ms or the telemetry goes stale. Gravity compensation, impedance, admittance and the gripper are not exposed yet.

> [!WARNING]
> Starting the launch file enables the arm. Clear the workspace and wire an independent hardware emergency stop first. Read [SAFETY.md](https://github.com/YaolinDong/infrabot/blob/main/SAFETY.md).

## Compatibility

Firmware 0.1.1 and SDK 0.1.3 with protocol 1.0, on ROS 2 Jazzy and Ubuntu 24.04.

[Apache-2.0](LICENSE) · [Changelog](CHANGELOG.md) · [Contributing](CONTRIBUTING.md)

Copyright © 2026 YaolinDong (董耀琳)
