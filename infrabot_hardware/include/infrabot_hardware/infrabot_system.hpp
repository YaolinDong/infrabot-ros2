// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Yaolin Dong
#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "hardware_interface/system_interface.hpp"
#include "hardware_interface/types/hardware_component_interface_params.hpp"
#include "hardware_interface/types/hardware_interface_return_values.hpp"
#include "infrabot/robot.hpp"
#include "rclcpp_lifecycle/state.hpp"

namespace infrabot_hardware
{

// ros2_control system for the H7C arm, J1-J7.
//
// configure: connect, wait for fresh telemetry, select the MIT drive profile.
// activate:  enable the arm, enter RT_JOINT and stream the commanded position
//            at 1 kHz through the SDK's native joint stream.
// deactivate: controlled stop; the arm stays enabled and holds.
// cleanup:   disconnect; the controller keeps holding the arm.
// The driver never disables the drives.
class InfrabotSystem : public hardware_interface::SystemInterface
{
public:
  static constexpr std::size_t kJoints = 7;
  using CallbackReturn = hardware_interface::CallbackReturn;

  CallbackReturn on_init(const hardware_interface::HardwareComponentInterfaceParams & params) override;
  CallbackReturn on_configure(const rclcpp_lifecycle::State & previous_state) override;
  CallbackReturn on_cleanup(const rclcpp_lifecycle::State & previous_state) override;
  CallbackReturn on_activate(const rclcpp_lifecycle::State & previous_state) override;
  CallbackReturn on_deactivate(const rclcpp_lifecycle::State & previous_state) override;
  CallbackReturn on_shutdown(const rclcpp_lifecycle::State & previous_state) override;
  CallbackReturn on_error(const rclcpp_lifecycle::State & previous_state) override;

  hardware_interface::return_type read(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;
  hardware_interface::return_type write(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

private:
  CallbackReturn check_info();
  bool wait_for_telemetry(std::string & error);
  void halt(bool wait);

  std::string robot_ip_ = "192.168.0.50";
  uint32_t telemetry_timeout_ms_ = 100;
  uint32_t command_timeout_ms_ = 100;

  std::unique_ptr<infrabot::Robot> robot_;
  bool active_ = false;
  infrabot::JointCommand command_;
  std::array<std::string, kJoints> position_state_, velocity_state_, effort_state_;
  std::array<std::string, kJoints> position_command_;
  std::string error_;
};

}  // namespace infrabot_hardware
