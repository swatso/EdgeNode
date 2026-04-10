# EdgeNode

IO and Edge Compute Node with Web Configuration and MQTT interface (eg to JMRI)

EdgeNode turns an ESP32 microcontroller into a smart, networked edge device for controlling GPIOs, servos, PWM outputs, audio playback, and user-defined action sequences — all configurable over WiFi via a browser and controllable via MQTT.

## Documentation

Full project documentation is available in the [`docs/`](docs/) directory:

- [Introduction & Quick Start](docs/Home.md)
- [Features Overview](docs/Features-Overview.md)
- [Installation Guide](docs/Installation-Guide.md)
- [Configuration](docs/Configuration.md)
- [Usage Examples](docs/Usage-Examples.md)
- [Contribution Guidelines](docs/Contribution-Guidelines.md)
- [License and Credits](docs/License-and-Credits.md)

## Quick Start

1. Clone this repository and open it in [PlatformIO](https://platformio.org/).
2. Set your WiFi credentials in `include/NodeServices.h`.
3. Upload the SPIFFS filesystem image (`pio run --target uploadfs`).
4. Build and flash the firmware (`pio run --target upload`).
5. Open `http://node00/page/node` in a browser to view the node status.
6. Navigate to `http://node00/page/nodeconfig` to set the Node ID and MQTT Broker IP.

## License

GNU General Public License v3.0 — see [LICENSE](LICENSE) for details.
