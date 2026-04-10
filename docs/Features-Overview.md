# Features Overview

EdgeNode provides a broad set of capabilities for edge compute and IO control on ESP32 hardware.

---

## GPIO Control

EdgeNode supports up to **16 configurable GPIO channels** with the following types:

| Type | Description |
|------|-------------|
| `GPIO_DIGIN` | Digital input — reads button/switch state and publishes via MQTT |
| `GPIO_DIGOUT` | Digital output — set high or low via MQTT or local logic |
| `GPIO_DIGOUT_PULSE` | Digital output with configurable pulse duration |
| `GPIO_PWM` | PWM output — intensity set as a numeric value (0–255) |
| `GPIO_PWM_PULSE` | PWM output with configurable on/off pulse cycle |
| `GPIO_SERVO` | Servo motor driven to a numeric position |
| `GPIO_SERVO_ACTUATOR` | Servo motor driven to one of three preset positions (open, centre, closed) |
| `GPIO_AIN` | Analogue input — reads a voltage level and publishes as a numeric value |

Each GPIO type maps to MQTT sensor and turnout topics so that any MQTT-capable controller (e.g., JMRI) can read inputs and set outputs over the network.

---

## Audio Playback

EdgeNode integrates an MP3 player module and exposes up to **16 configurable audio tracks**. Each track has:

- A friendly name (e.g., `"Ambient"`, `"Train Bell"`, `"Whistle"`)
- A duration (milliseconds) — used for automatic track completion handling
- A volume level (0–30)

Audio can be triggered remotely over MQTT or locally from within an action sequence (see below).

---

## Action Engine

The **action engine** is the core edge compute feature of EdgeNode. An action is a named, user-defined sequence of events (an "animation sequence") that can:

- Control GPIOs (set/clear outputs, change PWM duty cycle)
- Play audio tracks
- Publish MQTT messages
- Pause between steps using a non-blocking state-machine pattern

Up to **16 concurrent actions** (numbered 0–15) are supported. Each action runs on its own FreeRTOS task on CPU core 0, so actions execute in parallel without blocking one another or the main MQTT/WiFi stack.

Actions can be triggered by:
- An incoming MQTT message (`P` = play once, `L` = loop, `S` = stop)
- A local hardware input (e.g., a run switch)
- Another action (actions can call `play()` or `stop()` on each other)

User-defined `play` and `stop` functions are registered per action in `src/UserCode.cpp`.

---

## MQTT Integration

EdgeNode connects to a standard MQTT broker (tested with Mosquitto and JMRI). It publishes and subscribes to a structured topic hierarchy:

| Direction | Topic pattern | Purpose |
|-----------|--------------|---------|
| Subscribe | `iot/io/turnout/nnXX` | Control outputs (GPIO, servo) |
| Subscribe | `iot/io/sound/nnXX` | Trigger audio playback |
| Subscribe | `iot/io/action/nnXX` | Trigger action sequences |
| Subscribe | `iot/io/sound/autotrim` | Enable/disable auto volume trim |
| Publish | `iot/io/sensor/nnXX` | Report input state changes |
| Publish | `iot/debug/<nodeID>` | Per-node debug stream |
| Publish | `iot/operations/<nodeID>` | Per-node operational messages |
| Publish | `iot/debug/global` | Global debug stream |
| Publish | `iot/operations/global` | Global operational messages |

Where `nn` is the two-character hex Node ID (e.g., `01`) and `XX` is the zero-based channel index in hex (e.g., `00`–`0F`).

---

## Web Interface

EdgeNode hosts an **asynchronous web server** (ESPAsyncWebServer) that provides:

- **Node Status page** (`/page/node`) — live display of Node ID, IP address, WiFi uptime, RSSI, MQTT broker address, MQTT state, and MQTT uptime.
- **Node Configuration page** (`/page/nodeconfig`) — set the Node ID and MQTT Broker IP, with a one-click restart button.
- **GPIO page** (`/page/gpio`) — view and configure individual GPIO channels.
- **Action page** (`/page/action`) — view running actions and their internal state variables.
- **Sound page** (`/page/sound`) — view and configure audio tracks.

All configuration is persisted to SPIFFS (ESP32 file system) so it survives power cycles.

---

## Networking

- WiFi credentials (SSID / password) are stored in SPIFFS and loaded at boot.
- The node establishes a WiFi connection and then connects to the configured MQTT broker on port **1883**.
- MQTT reconnection is handled automatically on a 500 ms ticker.
- The node hostname defaults to `node00` (derived from the Node ID).

---

## Development Platform

| Component | Details |
|-----------|---------|
| Hardware | ESP32 Dev Kit (or compatible) |
| Build system | PlatformIO |
| Framework | Arduino for ESP32 |
| Key libraries | ESPAsyncWebServer, AsyncTCP, PubSubClient, ArduinoJson, ESP32Servo, ServoEasing, EspSoftwareSerial |
