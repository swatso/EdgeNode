# Usage Examples

This page covers the most common EdgeNode use cases with concrete examples drawn from the project source code.

---

## 1. Web Setup

### Accessing the Configuration Page

After flashing and booting the node:

1. Open a browser and go to `http://node00/page/nodeconfig` (replace `node00` with your hostname).
2. Enter the desired **Node ID** (a two-digit hex value from `00` to `FF`, e.g., `01`).
3. Enter the **MQTT Broker IP** address of your broker (e.g., `192.168.0.2`).
4. Click **Submit** to save the values to SPIFFS.
5. Click **Restart** to reboot the node and apply the new configuration.

After restarting, navigate to `http://node01/page/node` (updated hostname) to confirm the node is connected and the MQTT status shows "Connected".

---

## 2. MQTT Interface

EdgeNode uses a structured topic scheme where `nn` is the two-character hex Node ID (e.g., `01`) and `XX` is the channel index in hex (e.g., `00`–`0F`).

### Controlling Outputs (Turnout Topics)

EdgeNode subscribes to turnout topics to control GPIO outputs:

```
Topic:   iot/io/turnout/nnXX
Example: iot/io/turnout/0100
```

**Payload values:**

| Payload | Effect on `GPIO_DIGOUT` / `GPIO_PWM` | Effect on `GPIO_SERVO_ACTUATOR` |
|---------|--------------------------------------|---------------------------------|
| `T` (Thrown) | Set output HIGH | Move to `preset2` position |
| `M` (Middle) | — | Move to `preset1` position |
| Any other | Set output LOW | Move to `preset0` position |

For `GPIO_SERVO` type, the payload is interpreted as an integer servo position.

**Example** — turn on output 0 of node `01`:

```
mosquitto_pub -h 192.168.0.2 -t "iot/io/turnout/0100" -m "T"
```

### Reading Inputs (Sensor Topics)

When an input changes state, EdgeNode publishes to a sensor topic:

```
Topic:   iot/io/sensor/nnXX
Example: iot/io/sensor/0100
```

**Published payload:**

| GPIO Type | Value published |
|-----------|----------------|
| `GPIO_DIGIN` / `GPIO_DIGOUT` / `GPIO_PWM` | `ACTIVE` or `INACTIVE` |
| `GPIO_SERVO_ACTUATOR` | `ACTIVE` (preset2), `MIDDLE` (preset1), or `INACTIVE` (preset0) |
| `GPIO_SERVO` / `GPIO_AIN` / `GPIO_NONE` | Numeric value as string |

### Controlling Audio (Sound Topics)

```
Topic:   iot/io/sound/nnXX
Example: iot/io/sound/0105
```

| Payload | Effect |
|---------|--------|
| `P` | Play track `XX` once |
| `L` | Loop track `XX` |
| `S` (or any other) | Stop playback |

**Example** — play track 5 on node `01`:

```
mosquitto_pub -h 192.168.0.2 -t "iot/io/sound/0105" -m "P"
```

**Auto-trim** (global):

```
Topic: iot/io/sound/autotrim
Payload: T  (enable) or F (disable)
```

### Triggering Actions (Action Topics)

```
Topic:   iot/io/action/nnXX
Example: iot/io/action/0100
```

| Payload | Effect |
|---------|--------|
| `P` | Play action `XX` once |
| `L` | Loop action `XX` |
| `S` | Stop action `XX` |

**Example** — trigger action 0 on node `01`:

```
mosquitto_pub -h 192.168.0.2 -t "iot/io/action/0100" -m "P"
```

### Debug and Operations Streams

EdgeNode publishes diagnostic messages to:

| Topic | Description |
|-------|-------------|
| `iot/debug/<nodeID>` | Per-node debug messages (e.g., `iot/debug/01`) |
| `iot/operations/<nodeID>` | Per-node operational messages |
| `iot/debug/global` | Global debug stream (all nodes) |
| `iot/operations/global` | Global operations stream (all nodes) |

Subscribe to all debug output:

```
mosquitto_sub -h 192.168.0.2 -t "iot/#" -v
```

---

## 3. Configuring GPIO Channels

GPIO types and presets are set at compile time in `src/UserCode.cpp` inside `setupUserCode()`:

```cpp
// Set GPIO 0 to PWM pulse mode
gpio[0].setType(GPIO_PWM_PULSE);
gpio[0].preset0 = 250;   // Mark period in ms
gpio[0].preset1 = 20;    // Off PWM level (0–255)
gpio[0].preset2 = 200;   // On PWM level (0–255)
gpio[0].rate    = 1000;  // Pulse cycle period in ms
```

Other common types:

```cpp
gpio[1].setType(GPIO_DIGIN);          // Digital input
gpio[2].setType(GPIO_DIGOUT);         // Digital output
gpio[3].setType(GPIO_SERVO_ACTUATOR); // Servo with three presets
gpio[3].preset0 = 0;                  // Closed position (degrees)
gpio[3].preset1 = 90;                 // Centre position (degrees)
gpio[3].preset2 = 180;               // Open position (degrees)
```

---

## 4. Defining Action Sequences

Actions are the core edge compute feature. Each action has a user-defined `play` function and an optional `stop` function. Both are registered in `setupUserCode()`.

### Registering an Action

```cpp
// In setupUserCode() — src/UserCode.cpp
strcpy(action[0].name, "My Action");
action[0].setActionPlayFunction(myPlayFcn);
action[0].setActionStopFunction(myStopFcn);
```

### Writing a Play Function

Play functions must be **non-blocking**. Use `action[number].userState` as a state variable and `return <delayMs>` to yield control back to the scheduler:

```cpp
int myPlayFcn(uint8_t number)
{
    switch(action[number].userState)
    {
        case 1:
            // Step 1: turn on GPIO 0, change pulse rate
            gpio[0].rate = 1000;
            gpio[0].localWrite(true);
            localOperations.println("Action " + String(number) + ": Step 1");
            action[number].userState = 2;
            return 10000;  // wait 10 s before step 2

        case 2:
            // Step 2: change rate and play sound
            gpio[0].rate = 500;
            mp3.play(CMD_LOCAL, 5, false);  // play track 5
            localDebug.println("Action " + String(number) + ": Step 2");
            action[number].userState = 3;
            return 20000;  // wait 20 s before step 3

        case 3:
            // Step 3: clean up and finish
            gpio[0].localWrite(false);
            return 0;  // returning 0 signals the action is complete

        default:
            return 0;
    }
}
```

### Writing a Stop Function

The stop function is called when the action is interrupted mid-sequence:

```cpp
int myStopFcn(uint8_t number)
{
    gpio[0].localWrite(false);  // ensure output is off
    localOperations.println("Action " + String(number) + " stopped");
    return 0;  // returning 0 signals that stopping is complete
}
```

### Monitoring a Local Switch

To react to a hardware button, run a polling action continuously:

```cpp
// In setupUserCode()
strcpy(action[15].name, "Run Switch Monitor");
action[15].setActionPlayFunction(runSwitchHandler);
action[15].play(CMD_LOCAL, true);  // loop forever

int runSwitchHandler(uint8_t number)
{
    switch(action[number].userState)
    {
        case 1:
            if(run1Switch()) {
                action[0].play(CMD_LOCAL, true);
                action[number].userState = 2;
            }
            break;
        case 2:
            if(!run1Switch()) {
                action[0].stop(CMD_LOCAL);
                action[number].userState = 1;
            }
            break;
    }
    return 500;  // poll every 500 ms
}
```

---

## 5. Publishing Diagnostic Messages

Inside any action or user code function, use the debug/operations streams:

```cpp
localDebug.println("Debug message for this node");
localOperations.println("Operational message for this node");
globalDebug.println("Debug message visible to all nodes");
globalOperations.println("Operational message visible to all nodes");
```

These streams publish to MQTT topics automatically, making remote diagnostics straightforward.

---

## 6. JMRI Integration

EdgeNode is designed to integrate with [JMRI](https://www.jmri.org/) via its MQTT interface:

1. In JMRI, configure the **MQTT Connection** with your broker IP and port 1883.
2. Create **Turnouts** in JMRI with system names like `MTiot/io/turnout/0100` (node 01, channel 0).
3. Create **Sensors** in JMRI with system names like `MSiot/io/sensor/0100`.
4. Use JMRI Logix or Layout Editor to control turnouts and read sensors as usual — EdgeNode translates these to physical GPIO states automatically.
