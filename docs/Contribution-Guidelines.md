# Contribution Guidelines

Thank you for your interest in contributing to EdgeNode! Contributions of all kinds are welcome — bug reports, feature requests, documentation improvements, and code changes.

---

## Getting Started

1. **Fork** the repository on GitHub.
2. **Clone** your fork locally:
   ```bash
   git clone https://github.com/<your-username>/EdgeNode.git
   cd EdgeNode
   ```
3. Create a **feature branch** from `main`:
   ```bash
   git checkout -b feature/my-improvement
   ```
4. Make your changes, then push and open a **Pull Request** against `main`.

---

## Development Environment

- Install [Visual Studio Code](https://code.visualstudio.com/) with the [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode).
- Open the repository root folder in VS Code — PlatformIO will detect `platformio.ini` automatically and install all dependencies on first build.
- Use `pio run` to build and `pio run --target upload` to flash.
- Monitor serial output with `pio device monitor` at 115200 baud.

See the [Installation Guide](Installation-Guide.md) for full setup instructions.

---

## Project Structure

```
EdgeNode/
├── data/               # Web interface HTML files (uploaded to SPIFFS)
│   ├── node.html       # Node status page
│   ├── nodeConfig.html # Node configuration page
│   └── ...
├── include/            # Header files
│   ├── action.h        # edgeAction class API
│   ├── NodeServices.h  # MQTTnode class and default config
│   └── ...
├── src/                # C++ source files
│   ├── main.cpp        # Entry point — setup() and loop()
│   ├── UserCode.cpp    # User-defined GPIO, audio, and action configuration
│   ├── action.cpp      # Action engine implementation
│   ├── MQTTComms.cpp   # MQTT client and topic handling
│   ├── NodeServices.cpp# Configuration load/save
│   └── ...
├── test/               # Unit/integration tests
├── platformio.ini      # PlatformIO project configuration
└── LICENSE             # GNU GPL v3 licence
```

---

## Where to Make Changes

| What you want to do | Where to edit |
|--------------------|--------------|
| Change GPIO configuration or action sequences | `src/UserCode.cpp` |
| Modify MQTT topic structure | `src/MQTTComms.cpp` |
| Add new REST API endpoints | `src/WiFiManager.cpp` |
| Add new web pages | `data/*.html` |
| Modify the action engine | `src/action.cpp` / `include/action.h` |
| Change node configuration logic | `src/NodeServices.cpp` / `include/NodeServices.h` |
| Add audio features | `src/sound.cpp` / `include/sound.h` |

---

## Coding Conventions

- **Language**: C++17 (Arduino/ESP32 framework).
- **Formatting**: Follow the existing indentation style (2 spaces for ESP32 source files).
- **Naming**: Use camelCase for variables and functions, PascalCase for class names.
- **Non-blocking code**: Action `play` and `stop` functions **must not** use `delay()`. Implement all delays using the state-machine pattern and `return <delayMs>` (see [Usage Examples](Usage-Examples.md)).
- **Serial output**: Keep `Serial.print()` statements in place for debug builds; do not add new unconditional serial output in hot paths.
- **SPIFFS paths**: Declare file paths as `const char*` constants at the top of the file (see `src/NodeServices.cpp`).

---

## Submitting a Pull Request

1. Ensure the code **builds without errors** (`pio run`).
2. Test on hardware where possible — describe your hardware setup in the PR.
3. Write a clear PR description covering:
   - **What** changed and **why**
   - Any **breaking changes** to the MQTT topic scheme, API, or configuration format
   - **How to test** the change
4. Reference any related GitHub Issues (e.g., `Closes #12`).

---

## Reporting Bugs

Open a GitHub Issue and include:

- A description of the unexpected behaviour
- Steps to reproduce
- Expected vs. actual output
- Serial monitor output (if applicable)
- Your hardware configuration and PlatformIO environment details

---

## Feature Requests

Open a GitHub Issue with the label **enhancement**. Describe:

- The use case you are trying to solve
- How you envision the feature working
- Any relevant MQTT topics, API endpoints, or UI changes involved

---

## Code of Conduct

Please be respectful and constructive in all communications. We aim to maintain a welcoming environment for contributors of all experience levels.
