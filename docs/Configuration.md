# Configuration

EdgeNode stores its runtime configuration in the ESP32 SPIFFS file system and exposes a browser-based configuration interface for the most common settings.

---

## Configuration Files (SPIFFS)

The following files are read from SPIFFS at boot by `MQTTnode::loadConfig()` (in `src/NodeServices.cpp`):

| File | Key | Default |
|------|-----|---------|
| `/nodeID.txt` | Two-character hex Node ID | `00` |
| `/brokerIP.txt` | MQTT broker IP address | `192.168.0.2` |
| `/ssid.txt` | WiFi SSID | _(see `NodeServices.h`)_ |
| `/pass.txt` | WiFi password | _(see `NodeServices.h`)_ |

If a file is absent, the corresponding `#define default…` value from `include/NodeServices.h` is used:

```cpp
#define defaultBrokerIP "192.168.0.2"
#define defaultNodeID   "00"
#define defaultHostName "node00"
#define defaultSSID     "YourSSID"
#define defaultPassword "YourPassword"
```

> **Security note:** Always update `defaultSSID`, `defaultPassword`, and `defaultBrokerIP` to match your environment before building and deploying the firmware.

---

## Web Configuration Interface

### Node Status Page (`/page/node`)

Source: [`data/node.html`](../data/node.html)

The node status page is the entry point of the web interface. It provides a live read-only view of the node's current state, fetching data from the REST API on load.

**Displayed fields:**

| Field | API endpoint | Description |
|-------|-------------|-------------|
| NodeID (hex) | `GET /api/nodeid/value` | The two-character hex Node ID |
| Node IP Address | `GET /api/node/status` → `ipAddress` | Current IP address |
| WIFI Uptime (Min) | `GET /api/node/status` → `WIFIuptime` | Minutes since WiFi connected |
| RSSI | `GET /api/node/status` → `rssi` | WiFi signal strength (dBm) |
| MQTT Broker IP | `GET /api/brokerip/value` | Configured broker IP address |
| MQTT State | `GET /api/node/status` → `mqttState` | Connected / Disconnected |
| MQTT Uptime (Min) | `GET /api/node/status` → `mqttUptime` | Minutes since MQTT connected |

**JavaScript excerpt** (`data/node.html`):

```javascript
async function getNodeStatus() {
    var response = await fetch("//" + window.location.host + "/api/nodeid/value");
    var data = await response.json();
    var resp = JSON.parse(JSON.stringify(data));
    document.getElementById('NodeID').innerText = resp.value;

    response = await fetch("//" + window.location.host + "/api/node/status");
    data = await response.json();
    resp = JSON.parse(JSON.stringify(data));
    document.getElementById('WIFIuptime').innerText = resp.WIFIuptime;
    document.getElementById('IPAddress').innerText = resp.ipAddress;
    document.getElementById('RSSI').innerText = resp.rssi;
    document.getElementById('MQTTState').innerText = resp.mqttState;
    document.getElementById('MQTTUptime').innerText = resp.mqttUptime;
}
```

A **Configure** button navigates to the Node Configuration page.

---

### Node Configuration Page (`/page/nodeconfig`)

Source: [`data/nodeConfig.html`](../data/nodeConfig.html)

The configuration page lets you update the Node ID and MQTT Broker IP address without re-flashing the firmware. Changes are written to SPIFFS and take effect after a restart.

**Editable fields:**

| Field | Description | API endpoint |
|-------|-------------|-------------|
| MQTT NodeID (00h to FFh) | Two-character uppercase hex value | `POST /api/nodeid/value` |
| MQTT Broker IP | IP address of your MQTT broker | `POST /api/brokerip/value` |

**Buttons:**

- **Submit** — POSTs the current field values to their respective API endpoints.
- **Restart** — POSTs to `/api/node/restart` to reboot the node and apply changes.

**JavaScript excerpt** (`data/nodeConfig.html`):

```javascript
async function postNodeConfig() {
    var ele = document.getElementById('NodeID');
    var nodeID = ele.value.toUpperCase();
    var url = "//" + window.location.host + "/api/nodeid/value";
    var out = '{"value":"' + nodeID + '"}';
    postit(url, out);

    ele = document.getElementById('BrokerIP');
    url = "//" + window.location.host + "/api/brokerip/value";
    out = '{"value":"' + ele.value + '"}';
    postit(url, out);
}

async function postRestart() {
    var url = "//" + window.location.host + "/api/node/restart";
    postit(url, '{"value": 1}');
}
```

On page load (`init()`), the current values are fetched from the API and pre-populated into the input fields.

---

## REST API Reference

All endpoints return and accept `application/json`.

| Method | Endpoint | Description | Example response |
|--------|----------|-------------|-----------------|
| `GET` | `/api/nodeid/value` | Read Node ID | `{"value":"01"}` |
| `POST` | `/api/nodeid/value` | Set Node ID | Body: `{"value":"01"}` |
| `GET` | `/api/brokerip/value` | Read broker IP | `{"value":"192.168.0.2"}` |
| `POST` | `/api/brokerip/value` | Set broker IP | Body: `{"value":"192.168.0.2"}` |
| `GET` | `/api/hostname/value` | Read hostname | `{"value":"node01"}` |
| `GET` | `/api/node/status` | Read live node status | See below |
| `POST` | `/api/node/restart` | Restart the node | — |

### `/api/node/status` response

```json
{
  "WIFIuptime": 42,
  "ipAddress": "192.168.0.101",
  "rssi": -65,
  "mqttState": "Connected",
  "mqttUptime": 41
}
```

---

## User Code Configuration

GPIO types, audio tracks, and action sequences are configured at compile time in `src/UserCode.cpp`. See the [Usage Examples](Usage-Examples.md) page for full details and code snippets.
