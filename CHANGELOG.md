# Changelog

## 0.1.2 — 2026-11-24

- `infrabot.repos` pins SDK 0.1.3. Tested on the arm with firmware 0.1.1: `/joint_states` at 1000 Hz and joint trajectories reached.
- The mock-hardware launch test runs in its own ROS domain and checks that the controller manager it reaches loads only `mock_components/GenericSystem` before it sends a goal. Before, the test could reach a running arm in the default domain and move it.

## 0.1.1 — 2026-11-24

- The controller manager runs at 1 kHz instead of 500 Hz, so every trajectory sample reaches the SDK joint stream.

## 0.1.0 — 2026-11-24

- ros2_control system `infrabot_hardware/InfrabotSystem` for J1–J7: position commands streamed at 1 kHz through the SDK, position, velocity and effort states.
- Controlled stop on deactivation and errors. The driver never disables the drives.
- `infrabot_bringup`: URDF with ros2_control, mock hardware option, joint trajectory and forward position controllers, launch file.
- Plugin load test and a mock-hardware launch test.
