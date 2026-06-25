#include "GCodeControl.h"

#include "debugStream.h"
#include "marlin_handshake.h"
#include "MQTTComms.h"

#include <SPIFFS.h>
#include <cstring>
#include "freertos/semphr.h"

namespace {
HardwareSerial gcodeUart(1);  // UART1 (use 1 or 2 typically)
MarlinHandshake<> handshake(gcodeUart);
bool RFIDEnable = false; // Flag to enable or disable RFID reporting
constexpr uint32_t kPoseStableSaveMs = 5000;
constexpr TickType_t kPoseMonitorIntervalTicks = pdMS_TO_TICKS(500);
constexpr float kPoseCompareEpsilon = 0.001F;
constexpr uint32_t kMarlinAckTimeoutMs = 6000;
constexpr TickType_t kMarlinWaitSliceTicks = pdMS_TO_TICKS(20);

TaskHandle_t posePersistenceTaskHandle = nullptr;
SemaphoreHandle_t marlinSendMutex = nullptr;
Pose lastPersistedPoses[kObjectCount];
bool hasLastPersistedPose[kObjectCount] = {false};

bool posesAreDifferent(const Pose& a, const Pose& b)
{
  return (fabsf(a.x - b.x) > kPoseCompareEpsilon)
      || (fabsf(a.y - b.y) > kPoseCompareEpsilon)
      || (fabsf(a.heading - b.heading) > kPoseCompareEpsilon)
      || (a.forward != b.forward);
}

void buildPoseFilePath(int objectIndex, char* pathBuffer, size_t pathBufferSize)
{
  if ((pathBuffer == nullptr) || (pathBufferSize == 0))
  {
    return;
  }

  snprintf(pathBuffer, pathBufferSize, "/pose%d.txt", objectIndex);
}

bool savePoseToFile(int objectIndex, const Pose& pose)
{
  if ((objectIndex < 0) || (objectIndex >= kObjectCount))
  {
    return false;
  }

  char path[20];
  buildPoseFilePath(objectIndex, path, sizeof(path));

  File poseFile = SPIFFS.open(path, FILE_WRITE);
  if (!poseFile || poseFile.isDirectory())
  {
    localDebug.println("Failed to open pose file for write: " + String(path));
    return false;
  }

  poseFile.printf("%.3f,%.3f,%.3f,%d\n", pose.x, pose.y, pose.heading, pose.forward);
  poseFile.close();
  return true;
}

bool loadPoseFromFile(int objectIndex, Pose& pose)
{
  if ((objectIndex < 0) || (objectIndex >= kObjectCount))
  {
    return false;
  }

  char path[20];
  buildPoseFilePath(objectIndex, path, sizeof(path));

  File poseFile = SPIFFS.open(path, FILE_READ);
  if (!poseFile || poseFile.isDirectory())
  {
    return false;
  }

  char line[64];
  size_t len = poseFile.readBytesUntil('\n', line, sizeof(line) - 1);
  line[len] = '\0';
  poseFile.close();

  if (len == 0)
  {
    return false;
  }

  float x = kDefaultObjectPose.x;
  float y = kDefaultObjectPose.y;
  float heading = kDefaultObjectPose.heading;
  int forward = kDefaultObjectPose.forward;

  const int parsedCount = sscanf(line, "%f,%f,%f,%d", &x, &y, &heading, &forward);
  if (parsedCount < 3)
  {
    return false;
  }

  pose.x = x;
  pose.y = y;
  pose.heading = heading;
  pose.forward = (parsedCount >= 4) ? forward : kDefaultObjectPose.forward;
  return true;
}

void posePersistenceTask(void*)
{
  int trackedObjectIndex = -1;
  Pose trackedPose = kDefaultObjectPose;
  uint32_t lastPoseChangeMs = 0;
  bool pendingSave = false;

  for (;;)
  {
    const int idx = currentObjectIndex;
    if ((idx < 0) || (idx >= kObjectCount))
    {
      trackedObjectIndex = -1;
      pendingSave = false;
      vTaskDelay(kPoseMonitorIntervalTicks);
      continue;
    }

    const Pose currentPose = objectPoses[idx];
    const uint32_t nowMs = millis();

    if (trackedObjectIndex != idx)
    {
      trackedObjectIndex = idx;
      trackedPose = currentPose;
      lastPoseChangeMs = nowMs;
      pendingSave = true;
    }
    else if (posesAreDifferent(currentPose, trackedPose))
    {
      trackedPose = currentPose;
      lastPoseChangeMs = nowMs;
      pendingSave = true;
    }

    if (pendingSave && ((nowMs - lastPoseChangeMs) >= kPoseStableSaveMs))
    {
      if (!hasLastPersistedPose[idx] || posesAreDifferent(currentPose, lastPersistedPoses[idx]))
      {
        if (savePoseToFile(idx, currentPose))
        {
          lastPersistedPoses[idx] = currentPose;
          hasLastPersistedPose[idx] = true;
          localDebug.println("Saved stable pose for object " + String(idx));
        }
      }
      pendingSave = false;
    }

    vTaskDelay(kPoseMonitorIntervalTicks);
  }
}

void normalizeRFIDTag(const char* input, char* output, size_t outputSize)
{
  if ((output == nullptr) || (outputSize == 0))
  {
    return;
  }

  output[0] = '\0';
  if (input == nullptr)
  {
    return;
  }

  while ((*input == ' ') || (*input == '\t') || (*input == '\r') || (*input == '\n'))
  {
    ++input;
  }

  size_t len = 0;
  while ((input[len] != '\0') && (len < (outputSize - 1)))
  {
    output[len] = input[len];
    ++len;
  }
  output[len] = '\0';

  while ((len > 0) && ((output[len - 1] == ' ') || (output[len - 1] == '\t') || (output[len - 1] == '\r') || (output[len - 1] == '\n')))
  {
    output[--len] = '\0';
  }
}

bool parsePoseFromG1Line(char* line, float& x, float& y, float& bearing)
{
  if (line == nullptr)
  {
    return false;
  }

  while ((*line == ' ') || (*line == '\t'))
  {
    ++line;
  }

  if ((line[0] == '\0') || (line[0] == ';'))
  {
    return false;
  }

  char* inlineComment = strchr(line, ';');
  if (inlineComment != nullptr)
  {
    *inlineComment = '\0';
  }

  bool isG1Line = false;
  bool hasX = false;
  bool hasY = false;
  bool hasZ = false;

  char* token = strtok(line, " \t");
  while (token != nullptr)
  {
    if ((strcmp(token, "G1") == 0) || (strcmp(token, "G01") == 0))
    {
      isG1Line = true;
    }
    else if ((token[0] == 'X') && (token[1] != '\0'))
    {
      x = atof(&token[1]);
      hasX = true;
    }
    else if ((token[0] == 'Y') && (token[1] != '\0'))
    {
      y = atof(&token[1]);
      hasY = true;
    }
    else if ((token[0] == 'Z') && (token[1] != '\0'))
    {
      bearing = atof(&token[1]);
      hasZ = true;
    }

    token = strtok(nullptr, " \t");
  }

  return isG1Line && (hasX || hasY || hasZ);
}
}

namespace {
void syncObjectPose(int objectIndex, float x, float y, float bearing)
{
  if ((objectIndex < 0) || (objectIndex >= kObjectCount))
  {
    return; 
  }

  objectPoses[objectIndex].x = x;
  objectPoses[objectIndex].y = y;
  objectPoses[objectIndex].heading = bearing;
  gcodeObjects[objectIndex].loadPose(x, y, bearing);
  gcodeObjects[objectIndex].pose.forward = objectPoses[objectIndex].forward;
}
}

GCodeObject gcodeObjects[kObjectCount];
Pose objectPoses[kObjectCount];
int currentObjectIndex = -1;
int RFIDObjectIndex = -1;

void updatePoseFromLine(char* line);

void initGCodeControl(uint32_t baud, int8_t rxPin, int8_t txPin)
{
  gcodeUart.begin(baud, SERIAL_8N1, rxPin, txPin);

  if (marlinSendMutex == nullptr)
  {
    marlinSendMutex = xSemaphoreCreateMutex();
    if (marlinSendMutex == nullptr)
    {
      Serial.println("[initGCodeControl] ERROR: failed to create marlinSendMutex");
    }
  }

  // Initialize object poses from SPIFFS where available, otherwise use defaults.
  for (int i = 0; i < kObjectCount; ++i)
  {
    Pose loadedPose = kDefaultObjectPose;
    if (!loadPoseFromFile(i, loadedPose))
    {
      loadedPose = kDefaultObjectPose;
    }

    objectPoses[i] = loadedPose;
    syncObjectPose(i, loadedPose.x, loadedPose.y, loadedPose.heading);
    lastPersistedPoses[i] = loadedPose;
    hasLastPersistedPose[i] = true;
  }

  gcodeObjects[0].setName("Tarmac Layer");
  gcodeObjects[0].setRFIDTag("C6CB4A92");
  gcodeObjects[1].setName("JCB 3CX");
  gcodeObjects[1].setRFIDTag("RFID_1");
  gcodeObjects[2].setName("Roller");
  gcodeObjects[2].setRFIDTag("RFID_2");
  gcodeObjects[3].setName("Workmen Pipe");
  gcodeObjects[3].setRFIDTag("C6CB4A7B");

  if (posePersistenceTaskHandle == nullptr)
  {
    xTaskCreatePinnedToCore(posePersistenceTask,
                            "PosePersist",
                            3072,
                            nullptr,
                            1,
                            &posePersistenceTaskHandle,
                            1);
  }

  // Home the machine to establish correct coordinates before sending any G-code files
  sendGCodeFile("/home.gcode");
}

bool setCurrentObjectIndex(int newIndex)
{
  // Single point of entry to change the current object index, 
  // ensuring that the pose is saved before switching.
  
  if ((newIndex < 0) || (newIndex >= kObjectCount))
  {
    return false;
  }

  // De-select the previous object if any
  if ((currentObjectIndex >= 0) && (currentObjectIndex < kObjectCount))
  {
    for (int i = 0; i < 3; ++i)
    {
      MarlinSender(kObjectDeselectGCode[currentObjectIndex][i]);
    }
    // save its pose before switching to the new object
    objectPoses[currentObjectIndex].x = gcodeObjects[currentObjectIndex].pose.x;
    objectPoses[currentObjectIndex].y = gcodeObjects[currentObjectIndex].pose.y;
    objectPoses[currentObjectIndex].heading = gcodeObjects[currentObjectIndex].pose.heading;

    if (savePoseToFile(currentObjectIndex, objectPoses[currentObjectIndex]))
    {
      lastPersistedPoses[currentObjectIndex] = objectPoses[currentObjectIndex];
      hasLastPersistedPose[currentObjectIndex] = true;
      localDebug.println("Saved pose while switching away from object " + String(currentObjectIndex));
    }
  }

  currentObjectIndex = newIndex;
//  syncObjectPose(newIndex, objectPoses[newIndex].x, objectPoses[newIndex].y, objectPoses[newIndex].heading);


  //Send the G-code to move to the new object's pose
  char gcodeLine[128];
  snprintf(gcodeLine, sizeof(gcodeLine), "G1 X%.3f Y%.3f Z%.3f", objectPoses[newIndex].x, objectPoses[newIndex].y, objectPoses[newIndex].heading);
  MarlinSender(gcodeLine);

  // Send the G-code to select the new object
  for (int i = 0; i < 3; ++i)
  {
    MarlinSender(kObjectSelectGCode[newIndex][i]);
  }
return true;
}

void MarlinSender(const char* line) {
  if (line == nullptr)
  {
    Serial.println("[MarlinSender] ERROR: null line");
    return;
  }

  if (marlinSendMutex == nullptr)
  {
    marlinSendMutex = xSemaphoreCreateMutex();
    if (marlinSendMutex == nullptr)
    {
      Serial.println("[MarlinSender] ERROR: marlinSendMutex unavailable");
      return;
    }
  }

  if (xSemaphoreTake(marlinSendMutex, pdMS_TO_TICKS(10000)) != pdTRUE)
  {
    Serial.println("[MarlinSender] ERROR: timeout acquiring marlinSendMutex");
    return;
  }

  Serial.printf("[MarlinSender] begin line='%s'\n", line);

  const uint32_t waitStartMs = millis();
  uint32_t lastWaitLogMs = waitStartMs;
  size_t waitIterations = 0;

  while (!handshake.canSendNow()) {
    ++waitIterations;

    handshake.processInput(); // Process any incoming responses from Marlin.

    const uint32_t nowMs = millis();
    if (handshake.isAckStalled(kMarlinAckTimeoutMs, nowMs))
    {
      Serial.printf("[MarlinSender] WARNING: ACK stalled for %lu ms, resetting handshake state before line='%s'\n",
                    static_cast<unsigned long>(nowMs - waitStartMs),
                    line);
      localDebug.println("MarlinSender ACK stalled; resetting handshake before line: " + String(line));
      handshake.reset();
      break;
    }

    if ((nowMs - lastWaitLogMs) >= 1000)
    {
      Serial.printf("[MarlinSender] waiting canSendNow elapsed=%lu ms iterations=%u line='%s'\n",
                    static_cast<unsigned long>(nowMs - waitStartMs),
                    static_cast<unsigned>(waitIterations),
                    line);
      lastWaitLogMs = nowMs;
    }

    // Wait until it's safe to send the next command
    vTaskDelay(kMarlinWaitSliceTicks);
  }

  const uint32_t waitedMs = millis() - waitStartMs;
  if (waitIterations > 0)
  {
    Serial.printf("[MarlinSender] canSendNow after %lu ms (%u iterations)\n",
                  static_cast<unsigned long>(waitedMs),
                  static_cast<unsigned>(waitIterations));
  }

  Serial.println("[MarlinSender] sending line to handshake");
  handshake.sendLine(line);
  Serial.println("[MarlinSender] sendLine returned");
  publishReporterLine(line);
  Serial.println("Sent G-code to Marlin:");
  Serial.println(line);

  // Parse pose from a mutable copy because tokenization modifies the buffer.
  char poseLine[128];
  strncpy(poseLine, line, sizeof(poseLine) - 1);
  poseLine[sizeof(poseLine) - 1] = '\0';
  updatePoseFromLine(poseLine);

  vTaskDelay(100 / portTICK_PERIOD_MS);
  Serial.println("[MarlinSender] done");
  xSemaphoreGive(marlinSendMutex);
}

bool sendGCodeFile(const char* filePath)
{
  if ((filePath == nullptr) || (filePath[0] == '\0'))
  {
    localDebug.println("G-code file path is empty");
    Serial.println("[sendGCodeFile] ERROR: empty file path");
    return false;
  }

  Serial.printf("[sendGCodeFile] START file='%s'\n", filePath);

  File gcodeFile = SPIFFS.open(filePath, FILE_READ);
  if (!gcodeFile || gcodeFile.isDirectory())
  {
    localDebug.println("Failed to open G-code file: " + String(filePath));
    Serial.printf("[sendGCodeFile] ERROR: failed to open '%s'\n", filePath);
    return false;
  }

  localDebug.println("Sending G-code from file: " + String(filePath));
  Serial.printf("[sendGCodeFile] Opened '%s', starting stream\n", filePath);

  char lineBuffer[128];
  size_t sentLines = 0;
  size_t fileLineNumber = 0;

  while (gcodeFile.available())
  {
    ++fileLineNumber;
    size_t len = gcodeFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
    lineBuffer[len] = '\0';

    Serial.printf("[sendGCodeFile] L%u raw='%s' (len=%u)\n",
                  static_cast<unsigned>(fileLineNumber),
                  lineBuffer,
                  static_cast<unsigned>(len));

    while ((len > 0) && ((lineBuffer[len - 1] == '\r') || (lineBuffer[len - 1] == ' ') || (lineBuffer[len - 1] == '\t')))
    {
      lineBuffer[--len] = '\0';
    }

    size_t start = 0;
    while ((lineBuffer[start] == ' ') || (lineBuffer[start] == '\t'))
    {
      ++start;
    }

    char* line = &lineBuffer[start];
    if (line[0] == '\0')
    {
      Serial.printf("[sendGCodeFile] L%u skip: empty/whitespace\n", static_cast<unsigned>(fileLineNumber));
      continue;
    }

    if (line[0] == ';')
    {
      Serial.printf("[sendGCodeFile] L%u skip: full-line comment\n", static_cast<unsigned>(fileLineNumber));
      continue;
    }

    char* inlineComment = strchr(line, ';');
    if (inlineComment != nullptr)
    {
      *inlineComment = '\0';

      size_t trimmedLen = strlen(line);
      while ((trimmedLen > 0) && ((line[trimmedLen - 1] == ' ') || (line[trimmedLen - 1] == '\t')))
      {
        line[--trimmedLen] = '\0';
      }

      if (line[0] == '\0')
      {
        Serial.printf("[sendGCodeFile] L%u skip: inline comment removed all content\n", static_cast<unsigned>(fileLineNumber));
        continue;
      }
    }

    Serial.printf("[sendGCodeFile] L%u send='%s'\n", static_cast<unsigned>(fileLineNumber), line);
    MarlinSender(line);
    Serial.printf("[sendGCodeFile] L%u sent OK\n", static_cast<unsigned>(fileLineNumber));
    ++sentLines;
  }

  gcodeFile.close();
  localDebug.println("Finished G-code file send, lines sent: " + String(sentLines));
  Serial.printf("[sendGCodeFile] DONE file='%s' linesSent=%u\n", filePath, static_cast<unsigned>(sentLines));
  return true;
}

bool sendGCodeFileList(const char* listFilePath)
{
  if ((listFilePath == nullptr) || (listFilePath[0] == '\0'))
  {
    localDebug.println("G-code list file path is empty");
    return false;
  }

  File listFile = SPIFFS.open(listFilePath, FILE_READ);
  if (!listFile || listFile.isDirectory())
  {
    localDebug.println("Failed to open G-code list file: " + String(listFilePath));
    return false;
  }

  localDebug.println("Sending G-code files from list: " + String(listFilePath));

  char pathBuffer[128];
  size_t fileCount = 0;
  bool allSucceeded = true;

  while (listFile.available())
  {
    size_t len = listFile.readBytesUntil('\n', pathBuffer, sizeof(pathBuffer) - 1);
    pathBuffer[len] = '\0';

    while ((len > 0) && ((pathBuffer[len - 1] == '\r') || (pathBuffer[len - 1] == ' ') || (pathBuffer[len - 1] == '\t')))
    {
      pathBuffer[--len] = '\0';
    }

    size_t start = 0;
    while ((pathBuffer[start] == ' ') || (pathBuffer[start] == '\t'))
    {
      ++start;
    }

    char* gcodePath = &pathBuffer[start];
    if (gcodePath[0] == '\0')
    {
      continue;
    }

    if (gcodePath[0] == ';')
    {
      continue;
    }

    char* inlineComment = strchr(gcodePath, ';');
    if (inlineComment != nullptr)
    {
      *inlineComment = '\0';

      size_t trimmedLen = strlen(gcodePath);
      while ((trimmedLen > 0) && ((gcodePath[trimmedLen - 1] == ' ') || (gcodePath[trimmedLen - 1] == '\t')))
      {
        gcodePath[--trimmedLen] = '\0';
      }

      if (gcodePath[0] == '\0')
      {
        continue;
      }
    }

    ++fileCount;
    if (!sendGCodeFile(gcodePath))
    {
      localDebug.println("Failed while sending listed file: " + String(gcodePath));
      allSucceeded = false;
    }
  }

  listFile.close();
  localDebug.println("Finished G-code list send, files processed: " + String(fileCount));
  return allSucceeded;
}

void updatePoseFromFile(const char* file, int objectIndex)
{
    // Get the last 'G1' line of the named GCode file which should contain the final pose in the format: ;POSE:X,Y,BEARING
    File gcodeFile = SPIFFS.open(file, FILE_READ);
    if (!gcodeFile || gcodeFile.isDirectory())    {
        localDebug.println("Failed to open G-code file for pose update: " + String(file));
        return;
    }
    // Read through the file to find the last line starting with G1

    char lineBuffer[128];
    float x = 0, y = 0, bearing = 0;
    bool foundPose = false;
    while (gcodeFile.available())
    {
        size_t len = gcodeFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
        lineBuffer[len] = '\0';

      float parsedX = x;
      float parsedY = y;
      float parsedBearing = bearing;
      if (parsePoseFromG1Line(lineBuffer, parsedX, parsedY, parsedBearing))
        {
        x = parsedX;
        y = parsedY;
        bearing = parsedBearing;
        foundPose = true;
        }
    }

    if (foundPose)
    {
      syncObjectPose(objectIndex, x, y, bearing);
      localDebug.println("Updated pose for object " + String(gcodeObjects[objectIndex].name) + " to (" + String(x) + ", " + String(y) + ") bearing " + String(bearing));
    }
    gcodeFile.close();
}

void updatePoseFromLine(char* line)
{
  if (line == nullptr)
  {
    return;
  }

  if ((currentObjectIndex < 0) || (currentObjectIndex >= kObjectCount))
  {
    return;
  }

  float x = objectPoses[currentObjectIndex].x;
  float y = objectPoses[currentObjectIndex].y;
  float bearing = objectPoses[currentObjectIndex].heading;

  if (parsePoseFromG1Line(line, x, y, bearing))
  {
    syncObjectPose(currentObjectIndex, x, y, bearing);
  }
}


void GCodeObjectRFIDReporter(const char* rfidTag)
{
  if(RFIDEnable == false)
  {
    return; // RFID reporting is disabled, ignore the tag
  }
  // Called when an RFID tag is detected, to match it against the known GCode objects 
  char normalizedTag[sizeof(gcodeObjects[0].rfidTag)];
  normalizeRFIDTag(rfidTag, normalizedTag, sizeof(normalizedTag));

  char message[64];
  snprintf(message, sizeof(message), "GCode object with RFID tag: %s", normalizedTag);
  localDebug.println(message);

  for(int i = 0; i < 16; ++i)
  {
    int cmp = strcmp(gcodeObjects[i].rfidTag, normalizedTag);
    Serial.printf("Checking object %d, stored:'%s' incoming:'%s' cmp:%d\n", i, gcodeObjects[i].rfidTag, normalizedTag, cmp);
    if (cmp == 0)
    {
      snprintf(message, sizeof(message), "Matched GCode object: %s at (%.2f, %.2f) bearing %.2f", gcodeObjects[i].name, gcodeObjects[i].pose.x, gcodeObjects[i].pose.y, gcodeObjects[i].pose.heading);
      localDebug.println(message);
      //setCurrentObjectIndex(i);
      RFIDObjectIndex = i;
      break;
    }
  }
}

void loadGCodeObject()
{
    // This function is invoked by the user (typically via an MQTT event or by pressing a pushbutton)
    // It performs the following actions:-
    //      It streams file pathRFID1 to home the puck and move the object past the RFID reader
    //      If the reader has detected the object then it streams path pathRFID3 to home the puck
    //      If not then it streams the pathRFID2 which move the object closer to the RFID  reader 
    //  After this is done, if an object has been detected then its Pose is updated
    //  Object detectio caused by the RFID reporter will set the RFIDObjectIndex to the index of the detected object, otherwise it will remain -1

  RFIDObjectIndex = -1; // reset to indicate no object is currently being loaded
  RFIDEnable = true; // enable the RFID reader to detect the object
  sendGCodeFile("/pathRFID1.gcode");
  vTaskDelay(50000 / portTICK_PERIOD_MS);
  if(RFIDObjectIndex != -1)
  {
    sendGCodeFile("/pathRFID3.gcode");      // Tag has been detected, send the path to home the puck
    localDebug.println("Object detected after first move, sent path to home the puck");
  }
  else
  {
    sendGCodeFile("/pathRFID2.gcode");
    localDebug.println("No object detected after first move, sent alternate path to move closer to RFID reader");
  }
  vTaskDelay(50000 / portTICK_PERIOD_MS);
  if(RFIDObjectIndex != -1)
  {
    syncObjectPose(RFIDObjectIndex, 0, 0, 90);
    localDebug.println("Loaded GCode object: " + String(gcodeObjects[RFIDObjectIndex].name));

    // Now send the Start Of Day file for the object to move it to its starting position
    char startOfDayFile[32];
    snprintf(startOfDayFile, sizeof(startOfDayFile), "/startOfDay%d.gcode", RFIDObjectIndex);
    sendGCodeFile(startOfDayFile);
    updatePoseFromFile(startOfDayFile, RFIDObjectIndex);
  }
  RFIDEnable = false; // disable the RFID reader after loading the object
}

