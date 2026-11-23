// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Yaolin Dong
#include "infrabot_hardware/infrabot_system.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "rclcpp/logging.hpp"

namespace infrabot_hardware
{
namespace
{
constexpr uint8_t kArmMask = INFRABOT_ARM_MASK;

uint32_t parse_ms(
  const std::unordered_map<std::string, std::string> & params, const std::string & key,
  uint32_t fallback)
{
  const auto it = params.find(key);
  if (it == params.end()) return fallback;
  const unsigned long value = std::stoul(it->second);
  if (value == 0 || value > 1000) throw std::out_of_range(key + " must be 1-1000 ms");
  return static_cast<uint32_t>(value);
}

bool has(const std::vector<hardware_interface::InterfaceInfo> & list, const std::string & name)
{
  return std::any_of(list.begin(), list.end(), [&](const auto & i) { return i.name == name; });
}
}  // namespace

InfrabotSystem::CallbackReturn InfrabotSystem::on_init(
  const hardware_interface::HardwareComponentInterfaceParams & params)
{
  if (SystemInterface::on_init(params) != CallbackReturn::SUCCESS) return CallbackReturn::ERROR;
  return check_info();
}

InfrabotSystem::CallbackReturn InfrabotSystem::check_info()
{
  const auto & params = info_.hardware_parameters;
  try {
    if (const auto it = params.find("robot_ip"); it != params.end()) robot_ip_ = it->second;
    telemetry_timeout_ms_ = parse_ms(params, "telemetry_timeout_ms", telemetry_timeout_ms_);
    command_timeout_ms_ = parse_ms(params, "command_timeout_ms", command_timeout_ms_);
  } catch (const std::exception & e) {
    RCLCPP_ERROR(get_logger(), "Invalid hardware parameter: %s", e.what());
    return CallbackReturn::ERROR;
  }
  if (info_.joints.size() != kJoints) {
    RCLCPP_ERROR(get_logger(), "Expected %zu joints (J1-J7), got %zu", kJoints, info_.joints.size());
    return CallbackReturn::ERROR;
  }
  for (std::size_t i = 0; i < kJoints; ++i) {
    const auto & joint = info_.joints[i];
    if (joint.command_interfaces.size() != 1 ||
        joint.command_interfaces[0].name != hardware_interface::HW_IF_POSITION)
    {
      RCLCPP_ERROR(get_logger(), "Joint %s: the only command interface must be position",
        joint.name.c_str());
      return CallbackReturn::ERROR;
    }
    for (const char * state : {hardware_interface::HW_IF_POSITION, hardware_interface::HW_IF_VELOCITY,
                               hardware_interface::HW_IF_EFFORT})
    {
      if (!has(joint.state_interfaces, state)) {
        RCLCPP_ERROR(get_logger(), "Joint %s lacks the %s state interface", joint.name.c_str(), state);
        return CallbackReturn::ERROR;
      }
    }
    position_state_[i] = joint.name + "/" + hardware_interface::HW_IF_POSITION;
    velocity_state_[i] = joint.name + "/" + hardware_interface::HW_IF_VELOCITY;
    effort_state_[i] = joint.name + "/" + hardware_interface::HW_IF_EFFORT;
    position_command_[i] = position_state_[i];
  }
  return CallbackReturn::SUCCESS;
}

bool InfrabotSystem::wait_for_telemetry(std::string & error)
{
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
  while (robot_->state().telemetry_age_ms > telemetry_timeout_ms_) {
    if (std::chrono::steady_clock::now() >= deadline) {
      error = "no fresh telemetry from " + robot_ip_;
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return true;
}

InfrabotSystem::CallbackReturn InfrabotSystem::on_configure(const rclcpp_lifecycle::State &)
{
  robot_ = std::make_unique<infrabot::Robot>(robot_ip_);
  std::string error;
  if (!robot_->connect(error) || !wait_for_telemetry(error)) {
    RCLCPP_ERROR(get_logger(), "Connect to %s failed: %s", robot_ip_.c_str(), error.c_str());
    robot_.reset();
    return CallbackReturn::ERROR;
  }
  const auto info = robot_->info();
  if ((info.capabilities & INFRABOT_CAP_JOINT_RT) == 0) {
    RCLCPP_ERROR(get_logger(), "Firmware %s has no RT joint streaming", info.firmware.c_str());
    robot_.reset();
    return CallbackReturn::ERROR;
  }
  const auto s = robot_->state();
  const bool mit = s.profile_ready && s.profile == infrabot::MotorProfile::Mit;
  if (!mit) {
    if (s.enabled_mask != 0 || s.mode != infrabot::Mode::Idle) {
      RCLCPP_ERROR(get_logger(),
        "The drives are enabled without the MIT profile; return to IDLE and disable them first");
      robot_.reset();
      return CallbackReturn::ERROR;
    }
    if (!robot_->set_motor_profile(infrabot::MotorProfile::Mit, error)) {
      RCLCPP_ERROR(get_logger(), "MIT drive profile: %s", error.c_str());
      robot_.reset();
      return CallbackReturn::ERROR;
    }
  }
  RCLCPP_INFO(get_logger(), "Connected to %s, firmware %s, protocol %u.%u", robot_ip_.c_str(),
    info.firmware.c_str(), info.protocol_major, info.protocol_minor);
  return CallbackReturn::SUCCESS;
}
InfrabotSystem::CallbackReturn InfrabotSystem::on_activate(const rclcpp_lifecycle::State &)
{
  std::string error;
  if (!robot_ || !wait_for_telemetry(error)) {
    RCLCPP_ERROR(get_logger(), "Activate: %s", error.empty() ? "not connected" : error.c_str());
    return CallbackReturn::ERROR;
  }
  if (robot_->state().mode != infrabot::Mode::Idle && !robot_->stop(error)) {
    RCLCPP_ERROR(get_logger(), "Return to IDLE: %s", error.c_str());
    return CallbackReturn::ERROR;
  }
  if ((robot_->state().enabled_mask & kArmMask) != kArmMask) {
    RCLCPP_INFO(get_logger(), "Enabling J1-J7");
    if (!robot_->servo_on(infrabot::MotorGroup::Arm, error)) {
      RCLCPP_ERROR(get_logger(), "Enable J1-J7: %s", error.c_str());
      return CallbackReturn::ERROR;
    }
  }
  if (!robot_->switch_mode(infrabot::Mode::RtJoint, error)) {
    RCLCPP_ERROR(get_logger(), "Enter RT_JOINT: %s", error.c_str());
    return CallbackReturn::ERROR;
  }
  const auto hold = robot_->hold_command();
  command_ = infrabot::JointCommand{};
  std::copy_n(hold.begin(), kJoints, command_.position.begin());
  infrabot::JointStreamOptions options;
  options.command_timeout_ms = command_timeout_ms_;
  options.feedback_timeout_ms = telemetry_timeout_ms_;
  if (!robot_->start_joint_stream(command_, options, error)) {
    RCLCPP_ERROR(get_logger(), "Start the joint stream: %s", error.c_str());
    halt(true);
    return CallbackReturn::ERROR;
  }
  for (std::size_t i = 0; i < kJoints; ++i) set_command(position_command_[i], double(hold[i]));
  active_ = true;
  RCLCPP_INFO(get_logger(), "Streaming RT_JOINT from the hold position");
  return CallbackReturn::SUCCESS;
}

void InfrabotSystem::halt(bool wait)
{
  active_ = false;
  if (!robot_) return;
  robot_->stop_joint_stream();
  std::string error;
  if (!robot_->stop(error, wait)) RCLCPP_WARN(get_logger(), "Controlled stop: %s", error.c_str());
}

InfrabotSystem::CallbackReturn InfrabotSystem::on_deactivate(const rclcpp_lifecycle::State &)
{
  halt(true);
  return CallbackReturn::SUCCESS;
}

InfrabotSystem::CallbackReturn InfrabotSystem::on_cleanup(const rclcpp_lifecycle::State &)
{
  if (active_) halt(true);
  robot_.reset();
  return CallbackReturn::SUCCESS;
}

InfrabotSystem::CallbackReturn InfrabotSystem::on_shutdown(const rclcpp_lifecycle::State & state)
{
  return on_cleanup(state);
}

InfrabotSystem::CallbackReturn InfrabotSystem::on_error(const rclcpp_lifecycle::State &)
{
  halt(false);
  robot_.reset();
  return CallbackReturn::SUCCESS;
}

hardware_interface::return_type InfrabotSystem::read(const rclcpp::Time &, const rclcpp::Duration &)
{
  if (!robot_) return hardware_interface::return_type::OK;
  const infrabot::RobotState s = robot_->state();
  if (s.telemetry_age_ms > telemetry_timeout_ms_) {
    RCLCPP_ERROR_THROTTLE(get_logger(), *get_clock(), 1000, "Telemetry is %u ms old",
      s.telemetry_age_ms);
    return active_ ? hardware_interface::return_type::ERROR : hardware_interface::return_type::OK;
  }
  for (std::size_t i = 0; i < kJoints; ++i) {
    set_state(position_state_[i], double(s.joints.position[i]));
    set_state(velocity_state_[i], double(s.joints.velocity[i]));
    set_state(effort_state_[i], double(s.joints.torque[i]));
  }
  if (active_ && s.mode != infrabot::Mode::RtJoint) {
    RCLCPP_ERROR(get_logger(), "The controller left RT_JOINT (%s, stop reason %u)",
      infrabot::status_name(s.operational_status), unsigned(s.safety.stop_reason));
    return hardware_interface::return_type::ERROR;
  }
  return hardware_interface::return_type::OK;
}

hardware_interface::return_type InfrabotSystem::write(const rclcpp::Time &, const rclcpp::Duration &)
{
  if (!active_) return hardware_interface::return_type::OK;
  for (std::size_t i = 0; i < kJoints; ++i) {
    const double q = get_command<double>(position_command_[i]);
    if (std::isfinite(q)) command_.position[i] = static_cast<float>(q);
  }
  if (!robot_->servej(command_, error_)) {
    RCLCPP_ERROR(get_logger(), "Joint stream: %s", error_.c_str());
    return hardware_interface::return_type::ERROR;
  }
  return hardware_interface::return_type::OK;
}

}  // namespace infrabot_hardware

#include "pluginlib/class_list_macros.hpp"
PLUGINLIB_EXPORT_CLASS(infrabot_hardware::InfrabotSystem, hardware_interface::SystemInterface)
