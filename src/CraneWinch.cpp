#include "CraneWinch.h"

CraneWinch::CraneWinch(TwoWire& wire, uint8_t address, uint8_t commandRegister, uint8_t statusRegister)
    : wire_(wire), address_(address), commandRegister_(commandRegister), statusRegister_(statusRegister) {}

bool CraneWinch::sendCommand(CraneCommand command)
{
    const uint8_t commandValue = static_cast<uint8_t>(command);
    if (commandValue > static_cast<uint8_t>(CraneCommand::Home)) {
        Serial.print("Invalid command: ");
        return false;
    }

    wire_.beginTransmission(address_);
//    wire_.write(commandRegister_);
    wire_.write(commandValue);

Serial.print("Sending command: ");
Serial.println(commandValue);


    return wire_.endTransmission() == 0;
}

bool CraneWinch::writeTargetPulseCount(uint8_t position, int16_t pulseCount)
{
    if (position < 1 || position > 3 ||
        pulseCount < kTargetPulseCountMin || pulseCount > kTargetPulseCountMax) {
        return false;
    }
    wire_.beginTransmission(address_);
    wire_.write(static_cast<uint8_t>(kRegPosition1Target + position - 1));
    wire_.write(static_cast<uint8_t>(pulseCount & 0xFF));
    wire_.write(static_cast<uint8_t>((pulseCount >> 8) & 0xFF));
    return wire_.endTransmission() == 0;
}

bool CraneWinch::readTargetPulseCount(uint8_t position, int16_t& pulseCount)
{
    if (position < 1 || position > 3) {
        return false;
    }
    wire_.beginTransmission(address_);
    wire_.write(static_cast<uint8_t>(kRegPosition1Target + position - 1));
    if (wire_.endTransmission() != 0) {
        return false;
    }
    if (wire_.requestFrom(address_, static_cast<uint8_t>(2)) != 2 || wire_.available() < 2) {
        return false;
    }
    const uint8_t lo = static_cast<uint8_t>(wire_.read());
    const uint8_t hi = static_cast<uint8_t>(wire_.read());
    pulseCount = static_cast<int16_t>(lo | (hi << 8));
    return true;
}

bool CraneWinch::readStatus(CraneStatus& status)
{
    if (wire_.requestFrom(address_, static_cast<uint8_t>(1)) != 1 || wire_.available() < 1) {
        return false;
    }
    const uint8_t statusValue = static_cast<uint8_t>(wire_.read());
    if (statusValue > static_cast<uint8_t>(CraneStatus::ErrorCondition)) {
        return false;
    }
    status = static_cast<CraneStatus>(statusValue);
    return true;
}