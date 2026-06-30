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
    "G28",      // home all axes
    "M206 Z8",  // set Z home offset
    "G28",      // home all axes
    "G1 X0 Y0 Z0"
};

const char* const kObjectDeselectGCode[kObjectCount][3] = {
    {"M106 S0", "G4 S2", "G1 E0 F1800"},
    {"M106 S0", "G4 S2", "G1 E0 F1800"},
    {"M106 S0", "G4 S2", "G1 E0 F1800"},
    {"M106 S0", "G4 S2", "G1 E0 F1800"},
    {"M106 S0", "G4 S2", "G1 E0 F1800"},
    {"M106 S0", "G4 S2", "G1 E0 F1800"}
};

const char* const kObjectSelectGCode[kObjectCount][3] = {
    {"M117 Tarmac Layer", "M106 S255", "G4 S2"},
    {"M117 Road Roller", "M106 S255", "G4 S2"},
    {"M117 JCB", "M106 S255", "G4 S2"},
    {"M117 Object 3", "M106 S255", "G4 S2"},
    {"M117 Object 4", "M106 S255", "G4 S2"},
    {"M117 Object 5", "M106 S255", "G4 S2"},
};
