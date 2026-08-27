#ifndef GCODECONTROL_H
#define GCODECONTROL_H

#include <Arduino.h>
# include "GCodeObjectConfig.h"

void initGCodeControl(uint32_t baud = 250000, int8_t rxPin = 22, int8_t txPin = 23);
void setSpeedPercent(int speed);
void MarlinSender(const char* line);
// Diagnostic: sends a fixed sequence of simple G-code commands directly via the handshake's
// sendLine()/processInput(), logging raw send/ack timing to Serial, to help isolate handshake
// lockups independent of any specific higher-level function. Safe to call once, e.g. at the
// end of setup(), after initGCodeControl() has run.
void runMarlinHandshakeSelfTest();
bool runPath(int obj, int path);
bool runScene(int scene);
// New SD-path workflow: path must already be a complete, Marlin-ready SD path
// (e.g. "OBJ_1/PATH_0.GCO"). runSDPath() queues the request on a background task
// (non-blocking, safe from MQTT callbacks); runSDPathBlocking() is the synchronous
// implementation, exposed for callers that must block on completion.
bool runSDPath(const char* path);
bool runSDPathBlocking(const char* path);
bool sendGCodeFile(const char* filePath);
bool sendGCodeFileList(const char* listFilePath);
// Synchronous variants: block until the transfer completes. Use only from dedicated
// background tasks (e.g. RFID identify/select logic) that must sequence on completion;
// prefer the non-blocking sendGCodeFile()/sendGCodeFileList() everywhere else.
bool sendGCodeFileBlocking(const char* filePath);
bool sendGCodeFileListBlocking(const char* listFilePath);
void GCodeObjectRFIDReporter(const char* rfidTag);
void loadGCodeObject();

class GCodeObject
{
public:
  Pose pose;                     // Pose of the object in the world
    char name[20];                 // friendly name for this object
  char rfidTag[20];              // RFID tag associated with this object
    uint8_t collisionRadius;       // Collision radius in arbitrary units (0..100)
  boolean isLoaded;              // flag to indicate if the pose data is loaded and valid

        GCodeObject() : pose{0, 0, 90}, collisionRadius(0), isLoaded(false) {
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

    void setCollisionRadius(uint8_t newCollisionRadius) {
        collisionRadius = (newCollisionRadius > 100U) ? 100U : newCollisionRadius;
    }

};

extern GCodeObject gcodeObjects[kObjectCount]; // Array to hold up to 16 GCode objects
extern Pose objectPoses[kObjectCount];
extern int currentObjectIndex;
extern Pose currentPose;
extern void loadGCodeObject(int index);
void setSpeed(int speed);
#endif // GCODECONTROL_H