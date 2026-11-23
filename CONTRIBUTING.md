# Contributing

Report problems with the ROS 2 version, the firmware and SDK versions, the launch command and the log.

Build and test in a Jazzy workspace as in the [README](README.md), then run:

```bash
colcon test --packages-select infrabot_hardware infrabot_bringup
colcon test-result --verbose
```

Use English commit subjects of the form `type(scope): description`. Changes to the wire protocol belong in [infrabot-firmware](https://github.com/YaolinDong/infrabot-firmware) and [infrabot-sdk](https://github.com/YaolinDong/infrabot-sdk).
