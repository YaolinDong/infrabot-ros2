# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Yaolin Dong
cmake_minimum_required(VERSION 3.16)
project(infrabot_bringup NONE)

find_package(ament_cmake REQUIRED)

install(DIRECTORY config launch urdf DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_ros REQUIRED)
  find_package(launch_testing_ament_cmake REQUIRED)
  # Run in a free ROS domain (1-100) so the test never reaches a live arm in domain 0.
  add_launch_test(test/test_mock_hardware.py TIMEOUT 120
    RUNNER "${ament_cmake_ros_DIR}/run_test_isolated.py")
endif()

ament_package()
