# Node 28 - Building Site Diorama CNC interface Manager

This project provides a web-configurable interface for managing a building site diorama CNC setup using an ESP32-based EdgeNode. Specifically, it controls 
- the Marlin Based CNC controller via a serial connection.
- Forklift Pallet control capture and release via digital I/O.
- Specific animation soundtracks via the inbuilt MP3 player.
- Web-based configuration and control interface.
- MQTT integration for remote control and monitoring.-

# Actions

This instance of Edge Node does not implement the user actions defined in the default EdgeNode setup. Instead it implements a feature called 'scenes' which is build around executeing GCode sequences on SD card files stored in the Marlin-based CNC controller.
There is separate handling for each scene together with callbacks which are triggered at the end of execution of the scene Gcode file. This is separately documented in this project.
Scene names are configured in `initGCodeControl()` in `src/GCodeControl.cpp` and can contain up to 30 bytes (30 ASCII characters); longer names are truncated.

# EdgeNode

IO and Edge Compute Node with Web Configuration and MQTT interface (eg to JMRI)

EdgeNode turns an ESP32 microcontroller into a smart, networked edge device for controlling GPIOs, servos, PWM outputs, audio playback, and user-defined action sequences — all configurable over WiFi via a browser and controllable via MQTT.

This controller handles:
- Control of the feature Animations
- Execution of scene GCode sequences on the Marlin-based CNC controller
## Documentation

Full project documentation is available in the [`docs/`](docs/) directory:
Refer to the Edge Node documentation for more details on setting up and using the platform.
- [Introduction & Quick Start](docs/Home.md)
- [Features Overview](docs/Features-Overview.md)
- [Installation Guide](docs/Installation-Guide.md)
- [Configuration](docs/Configuration.md)
- [Usage Examples](docs/Usage-Examples.md)
- [Contribution Guidelines](docs/Contribution-Guidelines.md)
- [License and Credits](docs/License-and-Credits.md)
- [Scene Callbacks Documentation](SCENE_CALLBACKS.md)
- [Scene Callbacks Index](SCENE_CALLBACKS_INDEX.md)
- [Scene Callbacks Quick Reference](SCENE_CALLBACKS_QUICKREFERENCE.md)
- 

## Quick Start

1. Clone this repository and open it in [PlatformIO](https://platformio.org/).
2. Set your WiFi credentials in `include/NodeServices.h`.
3. Upload the SPIFFS filesystem image (`pio run --target uploadfs`).
4. Build and flash the firmware (`pio run --target upload`).
5. Open `http://node00/page/node` in a browser to view the node status.
6. Navigate to `http://node00/page/nodeconfig` to set the Node ID and MQTT Broker IP.

## License

GNU General Public License v3.0 — see [LICENSE](LICENSE) for details.
