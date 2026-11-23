// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Yaolin Dong
// Loads the plugin through the resource manager. on_init needs no robot, so
// the test never opens a socket: the component stays unconfigured.
#include <gmock/gmock.h>

#include <string>

#include "hardware_interface/resource_manager.hpp"
#include "rclcpp/rclcpp.hpp"

namespace
{
std::string urdf(const std::string & command = "position", const std::string & param = "")
{
  std::string s =
    "<?xml version=\"1.0\"?><robot name=\"h7c\"><link name=\"base_link\"/>";
  std::string parent = "base_link";
  for (int i = 1; i <= 7; ++i) {
    const std::string link = "Link_" + std::to_string(i);
    s += "<link name=\"" + link + "\"/><joint name=\"Joint_" + std::to_string(i) +
      "\" type=\"revolute\"><parent link=\"" + parent + "\"/><child link=\"" + link +
      "\"/><axis xyz=\"0 0 1\"/><limit lower=\"-1\" upper=\"1\" effort=\"10\" velocity=\"1\"/>"
      "</joint>";
    parent = link;
  }
  s += "<ros2_control name=\"InfrabotH7C\" type=\"system\"><hardware>"
    "<plugin>infrabot_hardware/InfrabotSystem</plugin>"
    "<param name=\"robot_ip\">127.0.0.1</param>" + param + "</hardware>";
  for (int i = 1; i <= 7; ++i) {
    s += "<joint name=\"Joint_" + std::to_string(i) + "\"><command_interface name=\"" + command +
      "\"/><state_interface name=\"position\"/><state_interface name=\"velocity\"/>"
      "<state_interface name=\"effort\"/></joint>";
  }
  return s + "</ros2_control></robot>";
}

class LoadInfrabotSystem : public ::testing::Test
{
protected:
  static void SetUpTestSuite() { rclcpp::init(0, nullptr); }
  static void TearDownTestSuite() { rclcpp::shutdown(); }
  rclcpp::Node::SharedPtr node_ = std::make_shared<rclcpp::Node>("test_infrabot_hardware");
};
}  // namespace

TEST_F(LoadInfrabotSystem, ExportsSevenJoints)
{
  hardware_interface::ResourceManager rm(urdf(), node_->get_clock(), node_->get_logger());
  for (int i = 1; i <= 7; ++i) {
    const std::string joint = "Joint_" + std::to_string(i);
    EXPECT_TRUE(rm.state_interface_exists(joint + "/position"));
    EXPECT_TRUE(rm.state_interface_exists(joint + "/velocity"));
    EXPECT_TRUE(rm.state_interface_exists(joint + "/effort"));
    EXPECT_TRUE(rm.command_interface_exists(joint + "/position"));
    EXPECT_FALSE(rm.command_interface_exists(joint + "/velocity"));
  }
  const auto & status = rm.get_components_status();
  ASSERT_EQ(status.count("InfrabotH7C"), 1u);
  EXPECT_EQ(status.at("InfrabotH7C").state.label(), "unconfigured");
}

// The resource manager logs a failed on_init and keeps running; it does not throw.
TEST_F(LoadInfrabotSystem, RejectsVelocityCommands)
{
  hardware_interface::ResourceManager rm(urdf("velocity"), node_->get_clock(), node_->get_logger());
  EXPECT_FALSE(rm.are_components_initialized());
  EXPECT_FALSE(rm.command_interface_exists("Joint_1/velocity"));
}

TEST_F(LoadInfrabotSystem, RejectsBadTimeout)
{
  hardware_interface::ResourceManager rm(
    urdf("position", "<param name=\"command_timeout_ms\">0</param>"), node_->get_clock(),
    node_->get_logger());
  EXPECT_FALSE(rm.are_components_initialized());
}
