# EdgeNode — Project Wiki

Welcome to the EdgeNode project wiki. EdgeNode is an **IO and Edge Compute Node** for ESP32-based hardware, providing a browser-based configuration interface and a full MQTT interface for integration with automation systems such as [JMRI](https://www.jmri.org/).

---

## Overview

EdgeNode turns an ESP32 microcontroller into a smart, networked edge device capable of:

- Controlling digital and analogue outputs (GPIOs, servos, PWM channels)
- Reading digital and analogue inputs and publishing their states via MQTT
- Playing audio tracks via an MP3 player module
- Running user-defined sequences of events ("actions") triggered by MQTT messages or local button presses
- Exposing a responsive web interface for live status monitoring and configuration

The firmware is built with [PlatformIO](https://platformio.org/) using the Arduino framework for ESP32.

---

## Wiki Sections

| Page | Description |
|------|-------------|
| [Features Overview](Features-Overview.md) | A summary of all major capabilities |
| [Installation Guide](Installation-Guide.md) | How to build and flash the firmware |
| [Configuration](Configuration.md) | Web UI and SPIFFS-based configuration reference |
| [Usage Examples](Usage-Examples.md) | MQTT interface, web setup, and action sequences |
| [Contribution Guidelines](Contribution-Guidelines.md) | How to contribute code, report bugs, and open PRs |
| [License and Credits](License-and-Credits.md) | Licensing information and acknowledgements |

---

## Quick Start

1. Clone the repository and open it in [PlatformIO](https://platformio.org/).
2. Flash the firmware to an ESP32 board.
3. On first boot the node uses the default WiFi credentials configured in `include/NodeServices.h` — update these to match your network before flashing.
4. Open a browser and navigate to `http://node00/page/node` to see the node status page.
5. Navigate to `http://node00/page/nodeconfig` to set the **Node ID** and **MQTT Broker IP**.
6. Restart the node and verify MQTT connectivity on your broker.

See the [Installation Guide](Installation-Guide.md) and [Configuration](Configuration.md) pages for full details.
