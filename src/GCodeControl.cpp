#include "GCodeControl.h"

#include "debugStream.h"
#include "marlin_handshake.h"
#include "MQTTServices.h"
#include "MQTTComms.h"

#include <SPIFFS.h>
#include <cstring>
#include "freertos/semphr.h"

//#define MarlinDebug
#define PrintMarlinLines

Pose currentPose = {0.0F, 0.0F, 90.0F};

namespace {
MQTTMessagePayload marlinLinePayload;
HardwareSerial gcodeUart(1);  // UART1 (use 1 or 2 typically)
MarlinHandshake<> handshake(gcodeUart);
bool RFIDEnable = false; // Flag to enable or disable RFID reporting
//constexpr uint32_t kPoseStableSaveMs = 5000;
//constexpr TickType_t kPoseMonitorIntervalTicks = pdMS_TO_TICKS(500);
constexpr float kPoseCompareEpsilon = 0.001F;
constexpr float kPoseMinX = 0.0F;
constexpr float kPoseMaxX = 265.0F;
constexpr float kPoseMinY = 0.0F;
constexpr float kPoseMaxY = 225.0F;
//constexpr uint32_t kMarlinAckTimeoutMs = 6000;
constexpr uint32_t kMarlinAckTimeoutMs = 600000;
constexpr uint32_t kMarlinExecutionWaitTimeoutMs = 30000;
constexpr TickType_t kMarlinWaitSliceTicks = pdMS_TO_TICKS(20);
int currentSpeedPercent = 100; // Default speed percentage for GCode movement
SemaphoreHandle_t marlinSendMutex = nullptr;

bool waitForMarlinCommandCompletion(const char* context, uint32_t timeoutMs = kMarlinExecutionWaitTimeoutMs)
{
  const uint32_t waitStartMs = millis();
  uint32_t lastLogMs = waitStartMs;

  while (!handshake.canSendNow())
  {
    handshake.processInput();

    const uint32_t nowMs = millis();
    if ((timeoutMs > 0) && ((nowMs - waitStartMs) >= timeoutMs))
    {
      localDebug.println(String("Timeout waiting for Marlin completion in ") + context);
      return false;
    }

#ifdef MarlinDebug
    if ((nowMs - lastLogMs) >= 1000)
    {
      Serial.printf("[MarlinWait] context=%s inflight=%u elapsed=%lu ms\n",
                    context,
                    static_cast<unsigned>(handshake.commandsInFlight()),
                    static_cast<unsigned long>(nowMs - waitStartMs));
      lastLogMs = nowMs;
    }
#endif

    vTaskDelay(kMarlinWaitSliceTicks);
  }

  // Keep ok bookkeeping from growing unnecessarily.
  while (handshake.consumeOk())
  {
  }

  return true;
}

float clampf(float value, float minValue, float maxValue)
{
  if (value < minValue)
  {
    return minValue;
  }
  if (value > maxValue)
  {
    return maxValue;
  }
  return value;
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

GCodeObject gcodeObjects[kObjectCount];

// Forward declarations
void updatePoseFromLine(char* line);

int RFIDObjectIndex = -1;

void initGCodeControl(uint32_t baud, int8_t rxPin, int8_t txPin)
{
  strncpy(marlinLinePayload.topic, reporterTopic, sizeof(marlinLinePayload.topic) - 1);
  marlinLinePayload.topic[sizeof(marlinLinePayload.topic) - 1] = '\0';
  gcodeUart.begin(baud, SERIAL_8N1, rxPin, txPin);

  if (marlinSendMutex == nullptr)
  {
    marlinSendMutex = xSemaphoreCreateMutex();
    if (marlinSendMutex == nullptr)
    {
#ifdef MarlinDebug
      Serial.println("[initGCodeControl] ERROR: failed to create marlinSendMutex");
#endif
    }
  }

  gcodeObjects[0].setName("Tarmac Layer");
  gcodeObjects[0].setRFIDTag("C6CB4A92");
  gcodeObjects[1].setName("JCB 3CX");
  gcodeObjects[1].setRFIDTag("RFID_1");
  gcodeObjects[2].setName("Roller");
  gcodeObjects[2].setRFIDTag("RFID_2");
  gcodeObjects[3].setName("Workmen Pipe");
  gcodeObjects[3].setRFIDTag("C6CB4A7B");
  gcodeObjects[4].setName("Workmen Board");
  gcodeObjects[4].setRFIDTag("C6CB4A67");
  gcodeObjects[5].setName("Cherry Picker");
  gcodeObjects[5].setRFIDTag("00EEEC2C");
  gcodeObjects[6].setName("Object6");
  gcodeObjects[6].setRFIDTag("00EEECC0");
  gcodeObjects[7].setName("Object7");
  gcodeObjects[7].setRFIDTag("00EEEC33");
  gcodeObjects[8].setName("Object8");
  gcodeObjects[8].setRFIDTag("00EEECC4");
  gcodeObjects[9].setName("Object9");
  gcodeObjects[9].setRFIDTag("00EEECD4");
  gcodeObjects[10].setName("Foreman");
  gcodeObjects[10].setRFIDTag("00EEECCE");
  gcodeObjects[11].setName("Jet1");
  gcodeObjects[11].setRFIDTag("00EEEC61");
  gcodeObjects[12].setName("Jet2");
  gcodeObjects[12].setRFIDTag("00EEEC68");
  gcodeObjects[13].setName("2 Workmen");
  gcodeObjects[13].setRFIDTag("00EEEC5F");
  gcodeObjects[14].setName("ShedA-door");
  gcodeObjects[14].setRFIDTag("00EEEC34");
  gcodeObjects[15].setName("ShedB-door");
  gcodeObjects[15].setRFIDTag("00EEEC76");

  // Home the machine to establish correct coordinates before sending any G-code files
  sendGCodeFile("/home.gcode");
}

void setSpeed(int speed)
{
  // Sets the feedrate for G-code movement. The speed is specified in mm/min.
  char gcodeLine[50];
  Serial.print("(setSpeed) speed:");
  Serial.println(speed);
  snprintf(gcodeLine, sizeof(gcodeLine), "G1 E0 F%d", speed);
  setSpeedPercent(currentSpeedPercent); // Ensure the speed percentage is applied after setting the feedrate
  MarlinSender(gcodeLine);
}

void setSpeedPercent(int speed)
{
  // Sets the feedrate for G-code movement. 
  // The speed is specified as a percentage of the current feedrate.
  char gcodeLine[50];
  Serial.print("(setSpeedPercent) speed%:");
  Serial.println(speed);
  snprintf(gcodeLine, sizeof(gcodeLine), "M220 S%d", speed);
  currentSpeedPercent = speed; // Update the current speed percentage
  MarlinSender(gcodeLine);
}

void MarlinSender(const char* line) {
  if (line == nullptr)
  {
#ifdef MarlinDebug
    Serial.println("[MarlinSender] ERROR: null line");
#endif
    return;
  }

  if (marlinSendMutex == nullptr)
  {
    marlinSendMutex = xSemaphoreCreateMutex();
    if (marlinSendMutex == nullptr)
    {
#ifdef MarlinDebug
      Serial.println("[MarlinSender] ERROR: marlinSendMutex unavailable");
#endif
      return;
    }
  }

  if (xSemaphoreTake(marlinSendMutex, pdMS_TO_TICKS(10000)) != pdTRUE)
  {
#ifdef MarlinDebug
    Serial.println("[MarlinSender] ERROR: timeout acquiring marlinSendMutex");
#endif
    return;
  }


  Serial.printf("[MarlinSender] begin line='%s'    ", line);


  const uint32_t waitStartMs = millis();
  uint32_t lastWaitLogMs = waitStartMs;
  size_t waitIterations = 0;

  while (!handshake.canSendNow()) {
    ++waitIterations;
    taskYIELD(); // Yield to allow other tasks to run while waiting for Marlin to be ready.
    handshake.processInput(); // Process any incoming responses from Marlin.

    const uint32_t nowMs = millis();
    if (handshake.isAckStalled(kMarlinAckTimeoutMs, nowMs))
    {
      Serial.printf("[MarlinSender] WARNING: ACK stalled for %lu ms, resetting handshake state before line='%s'\n",
                    static_cast<unsigned long>(nowMs - waitStartMs),
                    line);
      handshake.reset();
      break;
    }

    if ((nowMs - lastWaitLogMs) >= 1000)
    {
#ifdef MarlinDebug
      Serial.printf("[MarlinSender] waiting canSendNow elapsed=%lu ms iterations=%u line='%s'\n",
                    static_cast<unsigned long>(nowMs - waitStartMs),
                    static_cast<unsigned>(waitIterations),
                    line);
#endif
      lastWaitLogMs = nowMs;
    }

    // Wait until it's safe to send the next command
    vTaskDelay(kMarlinWaitSliceTicks);
  }

  const uint32_t waitedMs = millis() - waitStartMs;
  if (waitIterations > 0)
  {
#ifdef MarlinDebug
    Serial.printf("[MarlinSender] canSendNow after %lu ms (%u iterations)\n",
                  static_cast<unsigned long>(waitedMs),
                  static_cast<unsigned>(waitIterations));
#endif
  }

#ifdef MarlinDebug
  Serial.println("[MarlinSender] sending line to handshake");
#endif
  handshake.sendLine(line);
#ifdef MarlinDebug
  Serial.println("[MarlinSender] sendLine returned");
#endif
  strncpy(marlinLinePayload.message, line, sizeof(marlinLinePayload.message) - 1);
  marlinLinePayload.message[sizeof(marlinLinePayload.message) - 1] = '\0';
  MQTTPublishMessage(marlinLinePayload);
#ifdef PrintMarlinLines
  Serial.print("[MarlinSender] sent line: ");
  Serial.println(line);
#endif

  // Parse pose from a mutable copy because tokenization modifies the buffer.
  char poseLine[128];
  strncpy(poseLine, line, sizeof(poseLine) - 1);
  poseLine[sizeof(poseLine) - 1] = '\0';
  updatePoseFromLine(poseLine);

  vTaskDelay(100 / portTICK_PERIOD_MS);
#ifdef MarlinDebug
  Serial.println("[MarlinSender] done");
#endif
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

#ifdef MarlinDebug
  Serial.printf("[sendGCodeFile] START file='%s'\n", filePath);
#endif

  File gcodeFile = SPIFFS.open(filePath, FILE_READ);
  if (!gcodeFile || gcodeFile.isDirectory())
  {
    localDebug.println("Failed to open G-code file: " + String(filePath));
    Serial.printf("[sendGCodeFile] ERROR: failed to open '%s'\n", filePath);
    return false;
  }

  localDebug.println("Sending G-code from file: " + String(filePath));
#ifdef MarlinDebug
  Serial.printf("[sendGCodeFile] Opened '%s', starting stream\n", filePath);
#endif

  char lineBuffer[128];
  size_t sentLines = 0;
  size_t fileLineNumber = 0;

  auto processLine = [&](char* rawLine) {
    size_t len = strlen(rawLine);

#ifdef MarlinDebug
    Serial.printf("[sendGCodeFile] L%u raw='%s' (len=%u)\n",
                  static_cast<unsigned>(fileLineNumber),
                  rawLine,
                  static_cast<unsigned>(len));
#endif

    while ((len > 0) && ((rawLine[len - 1] == '\r') || (rawLine[len - 1] == ' ') || (rawLine[len - 1] == '\t')))
    {
      rawLine[--len] = '\0';
    }

    size_t start = 0;
    while ((rawLine[start] == ' ') || (rawLine[start] == '\t'))
    {
      ++start;
    }

    char* line = &rawLine[start];
    if (line[0] == '\0')
    {
#ifdef MarlinDebug
      Serial.printf("[sendGCodeFile] L%u skip: empty/whitespace\n", static_cast<unsigned>(fileLineNumber));
#endif
      return;
    }

    if (line[0] == ';')
    {
#ifdef MarlinDebug
      Serial.printf("[sendGCodeFile] L%u skip: full-line comment\n", static_cast<unsigned>(fileLineNumber));
#endif
      return;
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
#ifdef MarlinDebug
        Serial.printf("[sendGCodeFile] L%u skip: inline comment removed all content\n", static_cast<unsigned>(fileLineNumber));
#endif
        return;
      }
    }

#ifdef MarlinDebug
    Serial.printf("[sendGCodeFile] L%u send='%s'\n", static_cast<unsigned>(fileLineNumber), line);
#endif
    MarlinSender(line);
#ifdef MarlinDebug
    Serial.printf("[sendGCodeFile] L%u sent OK\n", static_cast<unsigned>(fileLineNumber));
#endif
    ++sentLines;
  };

  while (gcodeFile.available())
  {
    ++fileLineNumber;
    size_t len = gcodeFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
    lineBuffer[len] = '\0';
    processLine(lineBuffer);
  }

  gcodeFile.close();

  if (sentLines > 0)
  {
    // Flush Marlin planner so this function only returns once all queued motion is done.
    MarlinSender("M400");
    if (!waitForMarlinCommandCompletion("sendGCodeFile/M400"))
    {
      Serial.println("Timed out waiting for M400 completion after file send");
      return false;
    }
  }
  Serial.printf("[sendGCodeFile] DONE file='%s' linesSent=%u\n", filePath, static_cast<unsigned>(sentLines));
  return true;
}

bool sendGCodeFileList(const char* listFilePath)
{
  if ((listFilePath == nullptr) || (listFilePath[0] == '\0'))
  {
    Serial.println("G-code list file path is empty");
    return false;
  }

  File listFile = SPIFFS.open(listFilePath, FILE_READ);
  if (!listFile || listFile.isDirectory())
  {
    Serial.println("Failed to open G-code list file: " + String(listFilePath));
    return false;
  }

  Serial.println("Sending G-code files from list: " + String(listFilePath));

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
      Serial.println("Failed while sending listed file: " + String(gcodePath));
      allSucceeded = false;
    }
  }

  listFile.close();
  Serial.println("Finished G-code list send, files processed: " + String(fileCount));
  return allSucceeded;
}

void updatePoseFromLine(char* line)
{
  if (line == nullptr)
  {
    return;
  }

  float x = ::currentPose.x;
  float y = ::currentPose.y;
  float bearing = ::currentPose.heading;

  /*
  if (parsePoseFromG1Line(line, x, y, bearing))
  {
    syncObjectPose(currentObjectIndex, x, y, bearing);
  }
    */
   parsePoseFromG1Line(line, x, y, bearing);

  ::currentPose.x = x;
  ::currentPose.y = y;
  ::currentPose.heading = bearing;
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
      RFIDObjectIndex = i;
      break;
    }
  }
}

void loadGCodeObject()
{
  // This function is invoked by the user (typically via an MQTT event or by pressing a pushbutton)
  // It performs the following actions:
  //   1. Streams pathRFID1 to home the puck and move the object past the RFID reader.
  //   2. If the RFID reader detected the object, streams pathRFID3 to home the puck.
  //      Otherwise, streams pathRFID2 to move the object closer to the RFID reader.
  //   3. If an object was detected:
  //        - Resets its pose to the known loaded position (0, 0, heading 90).
  //        - Sends the start-of-day G-code file to move it to its starting position.

  RFIDObjectIndex = -1;
  RFIDEnable = true;    // enable the RFID reader to detect the object

  sendGCodeFile("/pathRFID1.gcode");

  if (RFIDObjectIndex != -1)
  {
    sendGCodeFile("/pathRFID3.gcode"); // Tag detected — home the puck
    localDebug.println("Object detected after first move, sent path to home the puck");
  }
  else
  {
    sendGCodeFile("/pathRFID2.gcode");
    localDebug.println("No object detected after first move, sent alternate path to move closer to RFID reader");
  }

  vTaskDelay(10000 / portTICK_PERIOD_MS);

  RFIDEnable = false; // disable the RFID reader now that detection window has closed

  if (RFIDObjectIndex != -1)
  {
    // Object successfully identified — select it and move it to its starting position.
    localDebug.println("Loaded GCode object: " + String(gcodeObjects[RFIDObjectIndex].name));

    char startOfDayFile[32];
    snprintf(startOfDayFile, sizeof(startOfDayFile), "/Path%d.0.gcode", RFIDObjectIndex);
    sendGCodeFile(startOfDayFile);
  }
  else
  {
    // No object detected
    localDebug.println("No object detected; restoring previous object selection");
  }
}

void loadGCodeObject(int index)
{
  // select object and move it to its starting position.
  localDebug.println("Loaded GCode object: " + String(gcodeObjects[index].name));
  char startOfDayFile[32];
  snprintf(startOfDayFile, sizeof(startOfDayFile), "/Path%d.0.gcode", index);
  sendGCodeFile(startOfDayFile);
}


