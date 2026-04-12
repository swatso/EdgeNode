# Installation Guide

This guide covers everything you need to build, flash, and run EdgeNode firmware on an ESP32 development board.

---

## Prerequisites

### Hardware

- An **ESP32 development board** (e.g., ESP32 DevKit v1 or compatible)
- USB cable for programming
- (Optional) MP3 player module (DFPlayer Mini or compatible) connected via SoftwareSerial
- (Optional) Servo motors, digital inputs/outputs as required by your use case

### Software

- [Visual Studio Code](https://code.visualstudio.com/) with the [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode), **or** the [PlatformIO Core CLI](https://docs.platformio.org/en/latest/core/installation/index.html)
- Git

---

## 1. Clone the Repository

```bash
git clone https://github.com/swatso/EdgeNode.git
cd EdgeNode
```

---

## 2. Review `platformio.ini`

The build configuration is defined in `platformio.ini`. The following build targets are available:

### Build targets

| Target | Controller | Board | Hardware macro | Description |
|--------|-----------|-------|----------------|-------------|
| `target1` | ESP32 | ESP32 Dev Kit | `HW_ESP32DEV` | Original PCB hardware — full GPIO complement, servos, audio |
| `target2` | ESP32-C3 | ESP32-C3 DevKitM-1 | `HW_ESP32C3` | Placeholder for future ESP32-C3 based PCB (pin assignments TBD) |

Shared library dependencies are declared once in the `[common]` section and inherited by all targets. Each target adds a `-D` preprocessor flag (`HW_ESP32DEV` or `HW_ESP32C3`) that source files use to select the correct GPIO pin assignments and any capability subsets.

```ini
[common]
framework = arduino
monitor_speed = 115200
lib_deps =
    esp32async/ESPAsyncWebServer@^3.9.4
    madhephaestus/ESP32Servo@^3.1.1
    arminjo/ServoEasing@^3.5.0
    bblanchon/ArduinoJson@^7.4.2
    esp32async/AsyncTCP@^3.4.10
    pubsubclient3@^3.3.0
    EspSoftwareSerial@^8.1.0

[env:target1]
platform = espressif32
board = esp32dev
framework = ${common.framework}
monitor_speed = ${common.monitor_speed}
lib_deps = ${common.lib_deps}
build_flags = -DHW_ESP32DEV

[env:target2]
platform = espressif32
board = esp32-c3-devkitm-1
framework = ${common.framework}
monitor_speed = ${common.monitor_speed}
lib_deps = ${common.lib_deps}
build_flags = -DHW_ESP32C3
```

PlatformIO will download all library dependencies automatically on the first build.

### Selecting a build target

**PlatformIO IDE (VS Code)**

Use the environment selector in the bottom status bar — click the target name and choose from the list.

**PlatformIO CLI**

```bash
# Build for Target 1 (ESP32 Dev Kit — original PCB hardware)
pio run -e target1

# Build for Target 2 (ESP32-C3 — future PCB hardware, placeholder)
pio run -e target2
```

### Adding a new hardware target

1. Add a new `[env:target<N>]` section in `platformio.ini` with the appropriate `board` and `build_flags = -DHW_<NAME>`.
2. Add the corresponding `#elif defined(HW_<NAME>)` block in `include/gpio.h` with the GPIO pin assignments for the new PCB design.
3. Use `#if defined(HW_<NAME>)` guards anywhere else in the source to include or exclude capabilities not supported by the new controller.

---

## 3. Configure Default WiFi Credentials

Before building, set your WiFi SSID and password in `include/NodeServices.h`:

```cpp
#define defaultSSID     "YourSSID"
#define defaultPassword "YourPassword"
```

> **Note:** These are only used as fallback defaults if no credentials have been saved to SPIFFS. Once the node has connected successfully, you can update the credentials via the file system or by re-flashing.

Optionally adjust the default Node ID and broker IP:

```cpp
#define defaultBrokerIP "192.168.0.2"
#define defaultNodeID   "00"
#define defaultHostName "node00"
```

---

## 4. Customise User Code (Optional)

Open `src/UserCode.cpp` to configure:

- **GPIO types and presets** — set each channel's type, PWM parameters, servo positions, etc.
- **Audio track configuration** — assign friendly names, durations, and volumes to up to 16 tracks.
- **Action sequences** — define `play` and `stop` functions for each action slot and register them.

See the [Usage Examples](Usage-Examples.md) page for worked examples.

---

## 5. Upload Web Interface Files to SPIFFS

The web interface HTML files in `data/` must be uploaded to the ESP32 SPIFFS partition separately from the firmware.

### Using PlatformIO IDE

In VS Code with PlatformIO:

1. Open the PlatformIO sidebar.
2. Under your target (e.g. `target1`), select **Platform → Upload Filesystem Image**.

### Using PlatformIO CLI

```bash
pio run --target uploadfs
```

> **Important:** Upload the SPIFFS image **before** or **after** flashing the firmware — not at the same time.

---

## 6. Build and Flash Firmware

### Using PlatformIO IDE

Click **Build** (✓) and then **Upload** (→) in the PlatformIO toolbar.

### Using PlatformIO CLI

```bash
pio run --target upload
```

---

## 7. Monitor Serial Output

Open the serial monitor at **115200 baud** to observe boot messages:

```bash
pio device monitor
```

A successful boot looks like:

```
(loadConfig) nodeIDString:00
(setupMQTTComms)
Connected to MQTT Broker at: 192.168.0.2
```

---

## 8. Access the Web Interface

Once the node is connected to WiFi, open a browser and navigate to:

```
http://node00/page/node
```

(Replace `node00` with your configured hostname if you changed `defaultHostName`.)

- **Node Status page:** `http://<hostname>/page/node`
- **Node Configuration page:** `http://<hostname>/page/nodeconfig`
- **GPIO page:** `http://<hostname>/page/gpio`
- **Action page:** `http://<hostname>/page/action`
- **Sound page:** `http://<hostname>/page/sound`

---

## Troubleshooting

| Symptom | Likely cause | Resolution |
|---------|-------------|------------|
| Node does not appear on the network | Wrong SSID/password | Check `defaultSSID` / `defaultPassword` in `NodeServices.h` |
| Web pages fail to load | SPIFFS not uploaded | Run `pio run --target uploadfs` |
| MQTT not connecting | Wrong broker IP | Set the correct IP in the configuration page or in `NodeServices.h` |
| Serial output stops after first MQTT attempt | Broker unreachable | Verify broker is running and IP is reachable from the ESP32 |
