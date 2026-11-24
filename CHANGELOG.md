# Changelog

## 0.1.0 — 2026-11-24

- ros2_control system `infrabot_hardware/InfrabotSystem` for J1–J7: position commands streamed at 1 kHz through the SDK, position, velocity and effort states.
- Controlled stop on deactivation and errors. The driver never disables the drives.
- `infrabot_bringup`: URDF with ros2_control, mock hardware option, joint trajectory and forward position controllers, launch file.
- Plugin load test and a mock-hardware launch test.
