#ifndef GCODECONTROL_H
#define GCODECONTROL_H

#include <Arduino.h>
# include "GCodeObjectConfig.h"

void initGCodeControl(uint32_t baud = 250000, int8_t rxPin = 22, int8_t txPin = 23);
void MarlinSender(const char* line);
bool sendGCodeFile(const char* filePath);
bool sendGCodeFileList(const char* listFilePath);
void GCodeObjectRFIDReporter(const char* rfidTag);
void loadGCodeObject();
bool setCurrentObjectIndex(int newIndex);

class GCodeObject
{
public:
  Pose pose;                     // Pose of the object in the world
      char name[20];                 // friendly name for this object
  char rfidTag[20];              // RFID tag associated with this object
  boolean isLoaded;              // flag to indicate if the pose data is loaded and valid

    GCodeObject() : pose{0, 0, 90}, isLoaded(false) {
        name[0] = '\0';
        rfidTag[0] = '\0';
    }

    void loadPose(float newX, float newY, float newBearing) {
        pose.x = newX;
        pose.y = newY;
        pose.heading = newBearing;
        isLoaded = true;
    }

    void setName(const char* newName) {
        strncpy(name, newName, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0'; // Ensure null-termination
    }

    void setRFIDTag(const char* newRFIDTag) {
        strncpy(rfidTag, newRFIDTag, sizeof(rfidTag) - 1);
        rfidTag[sizeof(rfidTag) - 1] = '\0'; // Ensure null-termination
    }

};

extern GCodeObject gcodeObjects[kObjectCount]; // Array to hold up to 16 GCode objects
extern Pose objectPoses[kObjectCount];
extern int currentObjectIndex;
extern void loadGCodeObject(int index);
#endif // GCODECONTROL_H