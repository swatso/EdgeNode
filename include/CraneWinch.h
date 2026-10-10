#ifndef CRANEWINCH_H
#define CRANEWINCH_H

#include <Arduino.h>
#include <Wire.h>

enum class CraneCommand : uint8_t {
    DisableAllMovement = 0x00,
    MoveToPosition1 = 0x01,
    MoveToPosition2 = 0x02,
    MoveToPosition3 = 0x03,
    Home = 0x04
};

enum class CraneStatus : uint8_t {
    DisableAllMovement = 0x00,
    Position1 = 0x01,
    Position2 = 0x02,
    Position3 = 0x03,
    Home = 0x04,
    Moving = 0x05,
    NotInitialised = 0x06,
    ErrorCondition = 0x07
};

class CraneWinch {
public:
    CraneWinch(TwoWire& wire, uint8_t address, uint8_t commandRegister, uint8_t statusRegister);

    bool sendCommand(CraneCommand command);
    bool readStatus(CraneStatus& status);

    // position is 1-3. Counts are int16 (0-300); the slave ignores out-of-range writes.
    bool readTargetPulseCount(uint8_t position, int16_t& pulseCount);
    bool writeTargetPulseCount(uint8_t position, int16_t pulseCount);

    static constexpr uint8_t kRegPosition1Target = 0x10;
    static constexpr int16_t kTargetPulseCountMin = 0;
    static constexpr int16_t kTargetPulseCountMax = 300;

private:
    TwoWire& wire_;
    uint8_t address_;
    uint8_t commandRegister_;
    uint8_t statusRegister_;
};

#endif // CRANEWINCH_H