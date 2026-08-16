#pragma once

#include <Arduino.h>


#include <cstring>

// Minimal Marlin line-ack handshake helper.
//
// Pattern:
// 1) sendLine() increments in-flight count.
// 2) processInput() parses CNC lines and decrements in-flight on "ok".
// 3) canSendNow() gates the next command.
template <size_t RxBufferSize = 256>
class MarlinHandshake {
 public:
  using LineHandler = void (*)(const char* line);

  explicit MarlinHandshake(HardwareSerial& cncSerial, Stream* debugStream = nullptr, LineHandler lineHandler = nullptr,
                            LineHandler rawLineHandler = nullptr)
      : cncSerial_(cncSerial), debugStream_(debugStream), lineHandler_(lineHandler), rawLineHandler_(rawLineHandler) {
    reset();
  }

  void reset() {
    rxLength_ = 0;
    rxBuffer_[0] = '\0';
    commandsInFlight_ = 0;
    okResponsesPending_ = 0;
    lastCommandSentMs_ = 0;
  }

  void setLineHandler(LineHandler handler) {
    lineHandler_ = handler;
  }

  // Fires for every line received (ok or not), independent of setLineHandler, for diagnostics/tracing.
  void setRawLineHandler(LineHandler handler) {
    rawLineHandler_ = handler;
  }

  void sendLine(const char* line) {
    cncSerial_.println(line);
    ++commandsInFlight_;
    lastCommandSentMs_ = millis();
  }

  void processInput() {
    while (cncSerial_.available() > 0) {
      const char ch = static_cast<char>(cncSerial_.read());

      if (ch == '\r' || ch == '\n') {
        if (rxLength_ > 0) {
          handleLine(rxBuffer_);
          rxLength_ = 0;
          rxBuffer_[0] = '\0';
        }
        continue;
      }

      if (rxLength_ < (RxBufferSize - 1)) {
        rxBuffer_[rxLength_++] = ch;
        rxBuffer_[rxLength_] = '\0';
      }
    }
  }

  bool canSendNow() const { return commandsInFlight_ == 0; }

  bool hasOkPending() const { return okResponsesPending_ > 0; }

  bool consumeOk() {
    if (okResponsesPending_ == 0) {
      return false;
    }
    --okResponsesPending_;
    return true;
  }

  uint16_t commandsInFlight() const { return commandsInFlight_; }

  uint16_t okResponsesPending() const { return okResponsesPending_; }

  uint32_t lastCommandSentMs() const { return lastCommandSentMs_; }

  bool isAckStalled(uint32_t timeoutMs, uint32_t nowMs) const {
    if (commandsInFlight_ == 0 || timeoutMs == 0) {
      return false;
    }
    return (nowMs - lastCommandSentMs_) >= timeoutMs;
  }

 private:
  static bool isOkResponse(const char* line) { return strncmp(line, "ok", 2) == 0; }

  void handleLine(const char* line) {
    if (rawLineHandler_ != nullptr) {
      rawLineHandler_(line);
    }

    if (isOkResponse(line)) {
      if (commandsInFlight_ > 0) {
        --commandsInFlight_;
      }
      ++okResponsesPending_;
    } else if (lineHandler_ != nullptr) {
      lineHandler_(line);
    }
  }

  HardwareSerial& cncSerial_;
  Stream* debugStream_;
  char rxBuffer_[RxBufferSize];
  size_t rxLength_ = 0;
  uint16_t commandsInFlight_ = 0;
  uint16_t okResponsesPending_ = 0;
  uint32_t lastCommandSentMs_ = 0;
  LineHandler lineHandler_ = nullptr;
  LineHandler rawLineHandler_ = nullptr;
};

