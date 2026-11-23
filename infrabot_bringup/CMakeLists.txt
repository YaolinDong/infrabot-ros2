# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Yaolin Dong
cmake_minimum_required(VERSION 3.16)
project(infrabot_bringup NONE)

find_package(ament_cmake REQUIRED)

install(DIRECTORY config launch urdf DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(launch_testing_ament_cmake REQUIRED)
  add_launch_test(test/test_mock_hardware.py TIMEOUT 120)
endif()

ament_package()
