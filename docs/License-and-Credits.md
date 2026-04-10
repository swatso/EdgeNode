# License and Credits

## License

EdgeNode is released under the **GNU General Public License v3.0 (GPL-3.0)**.

You are free to use, modify, and distribute this software under the terms of the GPL-3.0, provided that:

- Any modified versions are also distributed under the GPL-3.0.
- The original copyright notice and licence text are retained.
- Source code is made available when distributing binaries.

The full licence text is available in the [`LICENSE`](../LICENSE) file in the root of the repository, and at [https://www.gnu.org/licenses/gpl-3.0.html](https://www.gnu.org/licenses/gpl-3.0.html).

---

## Third-Party Libraries

EdgeNode depends on the following open-source libraries. Each library is subject to its own licence — please review them before commercial use.

| Library | Author / Maintainer | Licence | Used for |
|---------|--------------------|---------|---------|
| [ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer) | ESP32Async | LGPL-2.1 | Asynchronous HTTP web server |
| [AsyncTCP](https://github.com/ESP32Async/AsyncTCP) | ESP32Async | LGPL-3.0 | Asynchronous TCP layer for ESP32 |
| [PubSubClient](https://github.com/knolleary/pubsubclient) | Nick O'Leary | MIT | MQTT client |
| [ArduinoJson](https://arduinojson.org/) | Benoît Blanchon | MIT | JSON serialisation / deserialisation |
| [ESP32Servo](https://github.com/madhephaestus/ESP32Servo) | Kevin Harrington | LGPL-2.1 | Servo motor control on ESP32 |
| [ServoEasing](https://github.com/ArminJo/ServoEasing) | Armin Joachimsmeyer | MIT | Smooth servo motion profiles |
| [EspSoftwareSerial](https://github.com/plerup/espsoftwareserial) | Peter Lerup | LGPL-2.1 | Software serial for MP3 player communication |

---

## Acknowledgements

- The **JMRI Project** ([jmri.org](https://www.jmri.org/)) for the MQTT interface conventions and inspiration for the turnout / sensor topic scheme.
- The **Espressif ESP32 Arduino Core** team for the underlying Arduino-compatible ESP32 SDK.
- The **PlatformIO** team for the excellent cross-platform build and dependency management toolchain.

---

## Contributing

Contributions are welcome! See the [Contribution Guidelines](Contribution-Guidelines.md) for how to get involved.
