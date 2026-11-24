# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Yaolin Dong
"""Bring up the stack on mock hardware and follow a short trajectory."""
import time
import unittest

import launch
import launch_testing
import launch_testing.actions
import pytest
import rclpy
from builtin_interfaces.msg import Duration
from control_msgs.action import FollowJointTrajectory
from controller_manager_msgs.srv import ListControllers, ListHardwareComponents
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.actions import IncludeLaunchDescription
from launch_ros.substitutions import FindPackageShare
from launch.substitutions import PathJoinSubstitution
from rclpy.action import ActionClient
from sensor_msgs.msg import JointState
from trajectory_msgs.msg import JointTrajectoryPoint

JOINTS = [f'Joint_{i}' for i in range(1, 8)]
TARGET = [0.2, 0.5, -0.2, 0.8, 0.1, 0.3, -0.1]


@pytest.mark.launch_test
def generate_test_description():
    stack = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(PathJoinSubstitution(
            [FindPackageShare('infrabot_bringup'), 'launch', 'infrabot.launch.py'])),
        launch_arguments={'use_mock_hardware': 'true', 'launch_rviz': 'false'}.items())
    return launch.LaunchDescription([stack, launch_testing.actions.ReadyToTest()])


class TestMockHardware(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        rclpy.init()
        cls.node = rclpy.create_node('test_infrabot_mock')

    @classmethod
    def tearDownClass(cls):
        cls.node.destroy_node()
        rclpy.shutdown()

    def wait_for(self, predicate, timeout):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            rclpy.spin_once(self.node, timeout_sec=0.1)
            if predicate():
                return True
        return False

    def active_controllers(self):
        client = self.node.create_client(ListControllers, '/controller_manager/list_controllers')
        self.assertTrue(client.wait_for_service(timeout_sec=30.0))
        future = client.call_async(ListControllers.Request())
        rclpy.spin_until_future_complete(self.node, future, timeout_sec=5.0)
        return {c.name for c in future.result().controller if c.state == 'active'}

    def hardware_plugins(self):
        client = self.node.create_client(
            ListHardwareComponents, '/controller_manager/list_hardware_components')
        self.assertTrue(client.wait_for_service(timeout_sec=30.0))
        future = client.call_async(ListHardwareComponents.Request())
        rclpy.spin_until_future_complete(self.node, future, timeout_sec=5.0)
        return {c.plugin_name for c in future.result().component}

    def test_controllers_and_trajectory(self):
        # Refuse to send a goal unless the controller manager we reached runs mock hardware.
        self.assertEqual(self.hardware_plugins(), {'mock_components/GenericSystem'})
        wanted = {'joint_state_broadcaster', 'joint_trajectory_controller'}
        self.assertTrue(self.wait_for(lambda: wanted <= self.active_controllers(), 30.0))

        latest = {}
        self.node.create_subscription(
            JointState, '/joint_states', lambda m: latest.update(zip(m.name, m.position)), 10)
        self.assertTrue(self.wait_for(lambda: set(JOINTS) <= set(latest), 10.0))

        client = ActionClient(self.node, FollowJointTrajectory,
                              '/joint_trajectory_controller/follow_joint_trajectory')
        self.assertTrue(client.wait_for_server(timeout_sec=10.0))
        goal = FollowJointTrajectory.Goal()
        goal.trajectory.joint_names = JOINTS
        goal.trajectory.points = [JointTrajectoryPoint(positions=TARGET,
                                                       time_from_start=Duration(sec=1))]
        sent = client.send_goal_async(goal)
        rclpy.spin_until_future_complete(self.node, sent, timeout_sec=5.0)
        self.assertTrue(sent.result().accepted)
        done = sent.result().get_result_async()
        rclpy.spin_until_future_complete(self.node, done, timeout_sec=10.0)
        self.assertEqual(done.result().result.error_code, 0)

        reached = lambda: all(abs(latest[j] - q) < 1e-3 for j, q in zip(JOINTS, TARGET))
        self.assertTrue(self.wait_for(reached, 5.0))


@launch_testing.post_shutdown_test()
class TestShutdown(unittest.TestCase):
    def test_exit_codes(self, proc_info):
        launch_testing.asserts.assertExitCodes(
            proc_info, allowable_exit_codes=[0, -2, -15, 2, 15])
