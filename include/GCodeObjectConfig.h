#pragma once

#include <Arduino.h>

struct Pose {
  float x;
  float y;
  float heading;
};

constexpr uint8_t kObjectCount = 16;  //0..15 = selectable Objects

const Pose kDefaultObjectPose = {0.0F, 0.0F, 90.0F};

const char* const kInitGCode[] = {
    "G21",      // mm units
    "G90",      // absolute positioning
    "M106 S0",  // magnets off
    "G4 S2",    // wait 2 seconds
    "G28",      // home all axes
    "M206 Z8",  // set Z home offset
    "G28",      // home all axes
    "G1 X0 Y0 Z0"
};

const char* const kObjectDeselectGCode[3] = {
    "M106 S0", "G4 S2", "G1 E0 F1800"
};

const char* const kObjectSelectGCode[2] = {
    "M106 S255", "G4 S2"
};
