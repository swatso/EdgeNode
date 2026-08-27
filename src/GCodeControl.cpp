#include "GCodeControl.h"

#include "debugStream.h"
#include "marlin_handshake.h"
#include "MQTTServices.h"
#include "MQTTComms.h"
#include "WiFiManager.h"

#include <SPIFFS.h>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include "freertos/semphr.h"
#include "freertos/task.h"

#define MarlinDebug
//#define PrintMarlinLines

Pose currentPose = {0.0F, 0.0F, 90.0F};

namespace {
MQTTMessagePayload marlinLinePayload;
HardwareSerial gcodeUart(1);  // UART1 (use 1 or 2 typically)
void handleMarlinFeedbackLine(const char* line);
void traceMarlinRawLine(const char* line);
MarlinHandshake<> handshake(gcodeUart, nullptr, handleMarlinFeedbackLine, traceMarlinRawLine);
bool RFIDEnable = false; // Flag to enable or disable RFID reporting
//constexpr uint32_t kPoseStableSaveMs = 5000;
//constexpr TickType_t kPoseMonitorIntervalTicks = pdMS_TO_TICKS(500);
constexpr float kPoseCompareEpsilon = 0.001F;
constexpr float kPoseMinX = 0.0F;
constexpr float kPoseMaxX = 265.0F;
constexpr float kPoseMinY = 0.0F;
constexpr float kPoseMaxY = 225.0F;
constexpr uint32_t kMarlinAckTimeoutMs = 6000;
//constexpr uint32_t kMarlinAckTimeoutMs = 600000;
constexpr uint32_t kMarlinExecutionWaitTimeoutMs = 30000;
constexpr uint32_t kMarlinSDPrintTimeoutMs = 300000; // SD-hosted paths can run far longer than in-line commands
constexpr TickType_t kMarlinWaitSliceTicks = pdMS_TO_TICKS(20);
constexpr TickType_t kMarlinInputPollTicks = pdMS_TO_TICKS(25);
int currentSpeedPercent = 100; // Default speed percentage for GCode movement
SemaphoreHandle_t marlinSendMutex = nullptr;
SemaphoreHandle_t sceneRunMutex = nullptr;
TaskHandle_t marlinInputTaskHandle = nullptr;
constexpr size_t kMaxScenePathIndices = 64;
int scenePathIndices[kMaxScenePathIndices];
char scenePathExtensions[kMaxScenePathIndices][4];
size_t scenePathCount = 0;
volatile uint32_t sdPrintCompletionCount = 0; // incremented when Marlin reports an SD print has finished
volatile uint32_t sdPrintFailureCount = 0;    // incremented when Marlin reports an SD file error (e.g. open failed)
volatile uint32_t marlinRxLineCount = 0;      // every line received from Marlin, for diagnostics
volatile uint32_t lastMarlinRxMs = 0;         // millis() timestamp of the last line received
volatile bool traceMarlinRawLines = true;     // disabled during high-volume M20 responses
char currentSceneListingPrefix[24] = "";      // e.g. "SCN_2/", restricts M20 listing parsing to this folder

struct GCodeFileRequest
{
  char path[80];
  bool isList;
};
QueueHandle_t gcodeFileRequestQueue = nullptr;
TaskHandle_t gcodeFileTaskHandle = nullptr;

// path is expected to already be fully formed (e.g. "OBJ_1/PATH_0.GCO") by the caller.
struct SDPathRequest
{
  char path[64];
};
QueueHandle_t sdPathRequestQueue = nullptr;
TaskHandle_t sdPathTaskHandle = nullptr;

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

// Waits for Marlin's "Done printing file" message, which only appears once an SD-hosted
// file (started via M24) has actually finished executing on the controller. Unlike M400,
// this is not affected by M400 being dequeued ahead of the SD file's queued motion commands.
// Also fails fast (without waiting out the full timeout) if Marlin reports an SD file error.
bool waitForSDPrintDone(const char* context, uint32_t doneBaseline, uint32_t failureBaseline,
                         uint32_t timeoutMs = kMarlinSDPrintTimeoutMs)
{
  const uint32_t waitStartMs = millis();
  uint32_t lastLogMs = waitStartMs;
  const uint32_t rxCountAtStart = marlinRxLineCount;

  while (sdPrintCompletionCount == doneBaseline)
  {
    if (sdPrintFailureCount != failureBaseline)
    {
      Serial.printf("[waitForSDPrintDone] FAILED context=%s Marlin reported an SD file error, aborting wait\n", context);
      return false;
    }

    const uint32_t nowMs = millis();
    if ((timeoutMs > 0) && ((nowMs - waitStartMs) >= timeoutMs))
    {
      localDebug.println(String("Timeout waiting for SD print completion in ") + context);
      Serial.printf("[waitForSDPrintDone] TIMEOUT context=%s waited=%lu ms linesReceived=%u lastRx=%lu ms ago\n",
                    context,
                    static_cast<unsigned long>(nowMs - waitStartMs),
                    static_cast<unsigned>(marlinRxLineCount - rxCountAtStart),
                    static_cast<unsigned long>((lastMarlinRxMs == 0) ? 0 : (nowMs - lastMarlinRxMs)));
      return false;
    }

    if ((nowMs - lastLogMs) >= 1000)
    {
      Serial.printf("[waitForSDPrintDone] context=%s elapsed=%lu ms baseline=%u current=%u linesReceived=%u lastRx=%lu ms ago\n",
                    context,
                    static_cast<unsigned long>(nowMs - waitStartMs),
                    static_cast<unsigned>(doneBaseline),
                    static_cast<unsigned>(sdPrintCompletionCount),
                    static_cast<unsigned>(marlinRxLineCount - rxCountAtStart),
                    static_cast<unsigned long>((lastMarlinRxMs == 0) ? 0 : (nowMs - lastMarlinRxMs)));
      lastLogMs = nowMs;
    }

    vTaskDelay(kMarlinWaitSliceTicks);
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

bool parsePathIndexFromSceneLine(const char* line, const char* expectedPrefix, int& pathIndex,
                                 char* pathExtension, size_t pathExtensionSize)
{
  if ((line == nullptr) || (line[0] == '\0'))
  {
    return false;
  }

  const char* pathMarker = line;
  const size_t prefixLen = (expectedPrefix != nullptr) ? strlen(expectedPrefix) : 0;
  if (prefixLen > 0)
  {
    // M20 returns paths relative to the SD root. A bare PATH_x entry cannot be
    // associated with the requested folder, so reject it to avoid selecting a
    // path from another directory.
    if (strncmp(line, expectedPrefix, prefixLen) == 0)
    {
      pathMarker = line + prefixLen;
    }
    else
    {
      return false;
    }
    if (strncmp(pathMarker, "PATH_", 5) != 0)
    {
      return false;
    }
  }
  else
  {
    pathMarker = strstr(line, "PATH_");
    if (pathMarker == nullptr)
    {
      return false;
    }
  }

  const char* digits = pathMarker + 5;
  if (!isdigit(static_cast<unsigned char>(digits[0])))
  {
    return false;
  }

  char* endPtr = nullptr;
  const long parsedValue = strtol(digits, &endPtr, 10);
  if ((endPtr == digits) || (parsedValue < 0) || (parsedValue > 32767))
  {
    return false;
  }

  const char* extension = strstr(digits, ".GCO");
  const char* extensionName = "GCO";
  if (extension == nullptr)
  {
    extension = strstr(digits, ".gco");
  }
  if (extension == nullptr)
  {
    extension = strstr(digits, ".GCD");
    extensionName = "GCD";
  }
  if (extension == nullptr)
  {
    extension = strstr(digits, ".gcd");
    extensionName = "GCD";
  }
  if ((extension == nullptr) || (pathExtension == nullptr) || (pathExtensionSize < 4))
  {
    return false;
  }

  pathIndex = static_cast<int>(parsedValue);
  strncpy(pathExtension, extensionName, pathExtensionSize - 1);
  pathExtension[pathExtensionSize - 1] = '\0';
  return true;
}

void handleSceneListingLine(const char* line)
{
  if ((line == nullptr) || (line[0] == '\0'))
  {
    return;
  }

  int parsedPathIndex = -1;
  char parsedPathExtension[4];
  if (!parsePathIndexFromSceneLine(line, currentSceneListingPrefix, parsedPathIndex,
                                   parsedPathExtension, sizeof(parsedPathExtension)))
  {
    return;
  }

  for (size_t i = 0; i < scenePathCount; ++i)
  {
    if (scenePathIndices[i] == parsedPathIndex)
    {
      return;
    }
  }

  if (scenePathCount < kMaxScenePathIndices)
  {
    scenePathIndices[scenePathCount] = parsedPathIndex;
    strncpy(scenePathExtensions[scenePathCount], parsedPathExtension,
            sizeof(scenePathExtensions[scenePathCount]) - 1);
    scenePathExtensions[scenePathCount][sizeof(scenePathExtensions[scenePathCount]) - 1] = '\0';
    ++scenePathCount;
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

bool parsePoseFromM114Line(const char* line, float& x, float& y, float& bearing)
{
  if (line == nullptr)
  {
    return false;
  }

  const char* xToken = strstr(line, "X:");
  const char* yToken = strstr(line, "Y:");
  const char* zToken = strstr(line, "Z:");
  if ((xToken == nullptr) || (yToken == nullptr) || (zToken == nullptr))
  {
    return false;
  }

  char* endPtr = nullptr;

  const float parsedX = strtof(xToken + 2, &endPtr);
  if (endPtr == (xToken + 2))
  {
    return false;
  }

  const float parsedY = strtof(yToken + 2, &endPtr);
  if (endPtr == (yToken + 2))
  {
    return false;
  }

  const float parsedZ = strtof(zToken + 2, &endPtr);
  if (endPtr == (zToken + 2))
  {
    return false;
  }

  x = parsedX;
  y = parsedY;
  bearing = parsedZ;
  return true;
}

bool publishPoseAsG1(float x, float y, float bearing)
{
  MQTTMessagePayload posePayload;
  strncpy(posePayload.topic, reporterTopic, sizeof(posePayload.topic) - 1);
  posePayload.topic[sizeof(posePayload.topic) - 1] = '\0';
  snprintf(posePayload.message, sizeof(posePayload.message), "G1 X%.3f Y%.3f Z%.3f", x, y, bearing);

  if (MQTTMessageQueue == nullptr)
  {
    return false;
  }

  if (xQueueSend(MQTTMessageQueue, &posePayload, 0) != pdPASS)
  {
    Serial.println("[MarlinPose] WARNING: MQTT message queue full, pose not published");
    return false;
  }

  return true;
}

void publishPoseIfChanged(float x, float y, float bearing)
{
  if ((fabsf(currentPose.x - x) <= kPoseCompareEpsilon) &&
      (fabsf(currentPose.y - y) <= kPoseCompareEpsilon) &&
      (fabsf(currentPose.heading - bearing) <= kPoseCompareEpsilon))
  {
    return;
  }

  currentPose.x = x;
  currentPose.y = y;
  currentPose.heading = bearing;

  publishPoseAsG1(x, y, bearing);

#ifdef PrintMarlinLines
  Serial.printf("[MarlinPose] published pose G1 X%.3f Y%.3f Z%.3f\n", x, y, bearing);
#endif
}

void handleMarlinFeedbackLine(const char* line)
{
  if ((line != nullptr) && (strstr(line, "open failed") != nullptr))
  {
    ++sdPrintFailureCount;
    Serial.printf("[MarlinSender] Detected SD file error from Marlin: '%s', count=%u\n", line,
                  static_cast<unsigned>(sdPrintFailureCount));
    return;
  }

  if ((line != nullptr) && (strstr(line, "Done printing file") != nullptr))
  {
    ++sdPrintCompletionCount;
    Serial.printf("[MarlinSender] Detected 'Done printing file' from Marlin, count=%u\n",
                  static_cast<unsigned>(sdPrintCompletionCount));
    return;
  }

  float x = currentPose.x;
  float y = currentPose.y;
  float bearing = currentPose.heading;

  if (parsePoseFromM114Line(line, x, y, bearing))
  {
    publishPoseIfChanged(x, y, bearing);
  }
}

// Fires for every line Marlin sends (ok and non-ok alike); used to confirm the serial link
// is alive and to see the exact raw text when diagnosing why a specific message isn't matched.
void traceMarlinRawLine(const char* line)
{
  ++marlinRxLineCount;
  lastMarlinRxMs = millis();

#ifdef PrintMarlinLines
  if (traceMarlinRawLines)
  {
    Serial.printf("[MarlinRX] len=%u raw='%s'\n", static_cast<unsigned>((line != nullptr) ? strlen(line) : 0), (line != nullptr) ? line : "");
  }
#endif
}

void marlinInputTask(void* pvParameters)
{
  (void)pvParameters;

  while (true)
  {
    if ((marlinSendMutex != nullptr) && (xSemaphoreTake(marlinSendMutex, pdMS_TO_TICKS(5)) == pdTRUE))
    {
      handshake.processInput();
      xSemaphoreGive(marlinSendMutex);
    }

    vTaskDelay(kMarlinInputPollTicks);
  }
}

// Runs sendGCodeFile()/sendGCodeFileList() requests off the caller's thread, one at a time,
// so a single file/list is never interleaved with another and callers such as the MQTT
// callback never block waiting for a (potentially long) G-code transfer to complete.
void gcodeFileTask(void* pvParameters)
{
  (void)pvParameters;
  GCodeFileRequest request;

  for (;;)
  {
    if (xQueueReceive(gcodeFileRequestQueue, &request, portMAX_DELAY) == pdPASS)
    {
      const bool ok = request.isList ? sendGCodeFileListBlocking(request.path) : sendGCodeFileBlocking(request.path);
      if (!ok)
      {
        Serial.printf("[gcodeFileTask] ERROR: failed to send %s '%s'\n", request.isList ? "list" : "file", request.path);
      }
    }
  }
}

// Runs runSDPathBlocking() requests off the caller's thread, one at a time, mirroring
// gcodeFileTask()'s pattern so callers (e.g. MQTT callbacks) never block on the SD print.
void sdPathTask(void* pvParameters)
{
  (void)pvParameters;
  SDPathRequest request;

  for (;;)
  {
    if (xQueueReceive(sdPathRequestQueue, &request, portMAX_DELAY) == pdPASS)
    {
      if (!runSDPathBlocking(request.path))
      {
        Serial.printf("[sdPathTask] ERROR: failed to run SD path '%s'\n", request.path);
      }
    }
  }
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
  gcodeUart.setRxBufferSize(4096);
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

  if (sceneRunMutex == nullptr)
  {
    sceneRunMutex = xSemaphoreCreateMutex();
    if (sceneRunMutex == nullptr)
    {
      Serial.println("[initGCodeControl] ERROR: failed to create sceneRunMutex");
    }
  }

  if (marlinInputTaskHandle == nullptr)
  {
    xTaskCreatePinnedToCore(marlinInputTask, "Marlin Input", 3072, nullptr, 1, &marlinInputTaskHandle, 1);
  }

  if (gcodeFileRequestQueue == nullptr)
  {
    gcodeFileRequestQueue = xQueueCreate(8, sizeof(GCodeFileRequest));
    if (gcodeFileRequestQueue == nullptr)
    {
      Serial.println("[initGCodeControl] ERROR: failed to create gcodeFileRequestQueue");
    }
  }

  if ((gcodeFileTaskHandle == nullptr) && (gcodeFileRequestQueue != nullptr))
  {
    xTaskCreatePinnedToCore(gcodeFileTask, "GCode File", 4096, nullptr, 1, &gcodeFileTaskHandle, 1);
  }

  if (sdPathRequestQueue == nullptr)
  {
    sdPathRequestQueue = xQueueCreate(8, sizeof(SDPathRequest));
    if (sdPathRequestQueue == nullptr)
    {
      Serial.println("[initGCodeControl] ERROR: failed to create sdPathRequestQueue");
    }
  }

  if ((sdPathTaskHandle == nullptr) && (sdPathRequestQueue != nullptr))
  {
    xTaskCreatePinnedToCore(sdPathTask, "SD Path", 4096, nullptr, 1, &sdPathTaskHandle, 1);
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

  //MarlinSender("G28");  //debug

  setSpeedPercent(100); // Ensure the speed percentage is applied before homing
  runSDPath("System/HomePuck.GCO");
  // Run homing asynchronously via sceneRunTask so setup() (and MQTT/WiFi/webserver bring-up) isn't
  // blocked for the ~15-20s the SD homing scene can take - blocking here previously left the WiFi/
  // MQTT stack starved for CPU time long enough to leave the broker socket in a stuck EAGAIN state. 
 /* if (!queueSceneRun(100))
  {
    Serial.println("[initGCodeControl] ERROR: failed to queue boot-time homing scene (100)");
  } */
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

// Diagnostic self-test: sends a short, fixed sequence of simple, side-effect-free G-code
// commands directly via handshake.sendLine()/processInput(), bypassing MarlinSender's
// pose-parsing/MQTT-publish logic entirely, so a serial capture around this call shows
// only raw handshake behaviour (send time, ack time, lines received in between). Intended
// to be called once, e.g. at the very end of setup() after WiFi/MQTT/all tasks are already
// running, so the CPU/task load at the time of the test matches real power-up conditions
// where lockups have been observed. Look for "[MarlinSelfTest]" and "[MarlinRX]" lines
// (enable #define PrintMarlinLines above to also see raw RX trace) in the captured log.
void runMarlinHandshakeSelfTest()
{
  struct TestCommand
  {
    const char* line;
    uint32_t postAckDelayMs; // extra delay after this command acks before sending the next one
  };

  // Mix of commands that always reply instantly with no motion (M115/M400/M114/M105/M119),
  // one trivial real motion command that still goes through the planner (G4 P0), and a tight
  // burst of the same command sent back-to-back with no delay - this helps distinguish
  // "a specific command triggers it" from "rapid/overlapping sends trigger it".
  static const TestCommand kTestSequence[] = {
      {"M115", 250},  // firmware info - exercises a basic single-line reply
      {"M400", 250},  // finish planner moves - should ack ~instantly, nothing queued
      {"M114", 250},  // report position - exercises the existing pose-line handler path
      {"M105", 250},  // report temperature - harmless even with no thermistors configured
      {"M119", 250},  // report endstop states - read-only
      {"G4 P0", 250}, // zero-length dwell - a real motion/planner command, not just a query
      {"M115", 0},    // burst: no delay before next send
      {"M115", 0},    // burst: no delay before next send
      {"M115", 500},  // burst: last one gets a settle delay
  };

  if (marlinSendMutex == nullptr)
  {
    Serial.println("[MarlinSelfTest] ERROR: marlinSendMutex not initialised, skipping self-test");
    return;
  }

  Serial.println("[MarlinSelfTest] ==== starting Marlin handshake self-test ====");

  for (size_t i = 0; i < (sizeof(kTestSequence) / sizeof(kTestSequence[0])); ++i)
  {
    const TestCommand& cmd = kTestSequence[i];

    if (xSemaphoreTake(marlinSendMutex, pdMS_TO_TICKS(10000)) != pdTRUE)
    {
      Serial.printf("[MarlinSelfTest] #%u ERROR: timeout acquiring marlinSendMutex before sending '%s'\n",
                    static_cast<unsigned>(i), cmd.line);
      continue;
    }

    const uint32_t sendMs = millis();
    const uint32_t rxCountAtSend = marlinRxLineCount;
    Serial.printf("[MarlinSelfTest] #%u >>> sendLine('%s') inFlight=%u atMs=%lu\n",
                  static_cast<unsigned>(i), cmd.line,
                  static_cast<unsigned>(handshake.commandsInFlight()),
                  static_cast<unsigned long>(sendMs));

    handshake.sendLine(cmd.line);

    uint32_t lastHeartbeatMs = sendMs;
    while (!handshake.canSendNow())
    {
      handshake.processInput();

      const uint32_t nowMs = millis();
      if ((nowMs - lastHeartbeatMs) >= 250)
      {
        Serial.printf("[MarlinSelfTest] #%u ... waiting elapsed=%lu ms inFlight=%u rxLinesSinceSend=%u lastRxMs=%lu\n",
                      static_cast<unsigned>(i),
                      static_cast<unsigned long>(nowMs - sendMs),
                      static_cast<unsigned>(handshake.commandsInFlight()),
                      static_cast<unsigned>(marlinRxLineCount - rxCountAtSend),
                      static_cast<unsigned long>(lastMarlinRxMs));
        lastHeartbeatMs = nowMs;
      }

      if (handshake.isAckStalled(kMarlinAckTimeoutMs, nowMs))
      {
        Serial.printf("[MarlinSelfTest] #%u LOCKUP DETECTED: no ack for '%s' after %lu ms (rxLinesSinceSend=%u) - resetting handshake\n",
                      static_cast<unsigned>(i), cmd.line,
                      static_cast<unsigned long>(nowMs - sendMs),
                      static_cast<unsigned>(marlinRxLineCount - rxCountAtSend));
        handshake.reset();
        break;
      }

      vTaskDelay(kMarlinWaitSliceTicks);
    }

    const uint32_t ackMs = millis();
    Serial.printf("[MarlinSelfTest] #%u <<< ack (or reset) for '%s' after %lu ms\n",
                  static_cast<unsigned>(i), cmd.line,
                  static_cast<unsigned long>(ackMs - sendMs));

    xSemaphoreGive(marlinSendMutex);

    if (cmd.postAckDelayMs > 0)
    {
      vTaskDelay(pdMS_TO_TICKS(cmd.postAckDelayMs));
    }
  }

  Serial.println("[MarlinSelfTest] ==== self-test complete ====");
}

// Blocking implementation for runSDPath(): path must already be a complete, Marlin-ready
// SD path (e.g. "OBJ_1/PATH_0.GCO"); this function performs no formatting of its own.
bool runSDPathBlocking(const char* path)
{
  if ((path == nullptr) || (path[0] == '\0'))
  {
    Serial.println("[runSDPathBlocking] ERROR: empty path");
    return false;
  }

  char selectPathLine[80];
  snprintf(selectPathLine, sizeof(selectPathLine), "M23 %s", path);

  MarlinSender("M21");
  MarlinSender(selectPathLine);
  const uint32_t sdPrintBaseline = sdPrintCompletionCount;
  const uint32_t sdFailureBaseline = sdPrintFailureCount;
  MarlinSender("M24");

  if (!waitForSDPrintDone("runSDPathBlocking", sdPrintBaseline, sdFailureBaseline))
  {
    Serial.printf("[runSDPathBlocking] ERROR: timed out waiting for SD print completion path='%s'\n", path);
    return false;
  }

  MarlinSender("M400");
  if (!waitForMarlinCommandCompletion("runSDPathBlocking/M400"))
  {
    Serial.printf("[runSDPathBlocking] ERROR: timed out waiting for path completion path='%s'\n", path);
    return false;
  }
  Serial.printf("[runSDPathBlocking] completed path='%s'\n", path);
  MarlinSender("M114");
  return true;
}

// Queues a preformed SD path (e.g. "OBJ_1/PATH_0.GCO") to be run by the background sdPathTask;
// returns whether the request was accepted (queued), not whether the print has completed. Safe
// to call from any context, including MQTT callbacks, since it never blocks on the SD transfer.
bool runSDPath(const char* path)
{
  if ((path == nullptr) || (path[0] == '\0'))
  {
    Serial.println("[runSDPath] ERROR: empty path");
    return false;
  }

  if (sdPathRequestQueue == nullptr)
  {
    Serial.println("[runSDPath] ERROR: sdPathRequestQueue not initialised");
    return false;
  }

  SDPathRequest request;
  strncpy(request.path, path, sizeof(request.path) - 1);
  request.path[sizeof(request.path) - 1] = '\0';

  if (xQueueSend(sdPathRequestQueue, &request, 0) != pdPASS)
  {
    Serial.printf("[runSDPath] WARNING: queue full, dropped request for '%s'\n", path);
    return false;
  }
  return true;
}

bool runPath(int obj, int path)
{
  if ((obj < 0) || (path < 0))
  {
    Serial.printf("[runPath] ERROR: invalid args obj=%d path=%d\n", obj, path);
    return false;
  }

  char selectPathLine[64];
  snprintf(selectPathLine, sizeof(selectPathLine), "M23 OBJ_%d/PATH_%d.GCO", obj, path);

  MarlinSender("M21");
  MarlinSender(selectPathLine);
  const uint32_t sdPrintBaseline = sdPrintCompletionCount;
  const uint32_t sdFailureBaseline = sdPrintFailureCount;
  MarlinSender("M24");

  if (!waitForSDPrintDone("runPath", sdPrintBaseline, sdFailureBaseline))
  {
    Serial.printf("[runPath] ERROR: timed out waiting for SD print completion obj=%d path=%d\n", obj, path);
    return false;
  }

  MarlinSender("M400");
  if (!waitForMarlinCommandCompletion("runPath/M400"))
  {
    Serial.printf("[runPath] ERROR: timed out waiting for path completion obj=%d path=%d\n", obj, path);
    return false;
  }
  Serial.printf("[runPath] completed obj=%d path=%d\n", obj, path);
  MarlinSender("M114");
  return true;
}

bool runScenePath(int obj, int path, const char* extension)
{
  if ((obj < 0) || (path < 0))
  {
    Serial.printf("[runScenePath] ERROR: invalid args obj=%d path=%d\n", obj, path);
    return false;
  }

  char selectPathLine[64];
  snprintf(selectPathLine, sizeof(selectPathLine), "M23 SCN_%d/PATH_%d.%s", obj, path,
           (extension != nullptr) ? extension : "GCO");
  Serial.print("[runScenePath] selectPathLine: ");
  Serial.println(selectPathLine);
  MarlinSender("M21");
  MarlinSender(selectPathLine);
  const uint32_t sdPrintBaseline = sdPrintCompletionCount;
  const uint32_t sdFailureBaseline = sdPrintFailureCount;
  MarlinSender("M24");

  if (!waitForSDPrintDone("runScenePath", sdPrintBaseline, sdFailureBaseline))
  {
    Serial.printf("[runScenePath] ERROR: timed out waiting for SD print completion obj=%d path=%d\n", obj, path);
    return false;
  }

  MarlinSender("M400");
  if (!waitForMarlinCommandCompletion("runScenePath/M400"))
  {
    Serial.printf("[runScenePath] ERROR: timed out waiting for path completion obj=%d path=%d\n", obj, path);
    return false;
  }
  Serial.printf("[runScenePath] completed obj=%d path=%d\n", obj, path);
  MarlinSender("M114");
  return true;
}

bool runScene(int scene)
{
  if ((sceneRunMutex != nullptr) && (xSemaphoreTake(sceneRunMutex, portMAX_DELAY) != pdTRUE))
  {
    Serial.println("[runScene] ERROR: failed to acquire scene execution mutex");
    return false;
  }

  auto releaseSceneRunMutex = [&]() {
    if (sceneRunMutex != nullptr)
    {
      xSemaphoreGive(sceneRunMutex);
    }
  };

  if (scene < 0)
  {
    Serial.printf("[runScene] ERROR: invalid scene index %d\n", scene);
    releaseSceneRunMutex();
    return false;
  }

  scenePathCount = 0;
  handshake.setLineHandler(handleSceneListingLine);

  char folderName[24];
  snprintf(folderName, sizeof(folderName), "SCN_%d", scene);
  snprintf(currentSceneListingPrefix, sizeof(currentSceneListingPrefix), "%s/", folderName);

  char listCommand[48];
  // M20 lists the complete card; Marlin does not support a folder argument.
  snprintf(listCommand, sizeof(listCommand), "M20");

  MarlinSender("M21");
  if (!waitForMarlinCommandCompletion("runScene/M21"))
  {
    Serial.printf("[runScene] ERROR: failed to mount SD card while listing scene %d\n", scene);
    handshake.setLineHandler(handleMarlinFeedbackLine);
    releaseSceneRunMutex();
    return false;
  }

  // M20 can produce the whole card in one burst. Avoid spending UART service time
  // printing each response line while the listing is still arriving. Retry once if
  // a transient RX overrun loses the command acknowledgement.
  bool listingCompleted = false;
  for (int attempt = 0; attempt < 2; ++attempt)
  {
    traceMarlinRawLines = false;
    MarlinSender(listCommand);
    listingCompleted = waitForMarlinCommandCompletion("runScene/M20");
    traceMarlinRawLines = true;
    if (listingCompleted)
    {
      break;
    }

    if (attempt == 0)
    {
      Serial.printf("[runScene] M20 listing acknowledgement lost for %s; retrying\n", folderName);
      handshake.reset();
      scenePathCount = 0;
      MarlinSender("M21");
      if (!waitForMarlinCommandCompletion("runScene/M21 retry"))
      {
        break;
      }
    }
  }

  if (!listingCompleted)
  {
    Serial.printf("[runScene] ERROR: timed out listing scene folder %s\n", folderName);
    handshake.setLineHandler(handleMarlinFeedbackLine);
    releaseSceneRunMutex();
    return false;
  }

  if (scenePathCount == 0)
  {
    Serial.printf("[runScene] ERROR: no PATH_n.GCO files found in %s\n", folderName);
    handshake.setLineHandler(handleMarlinFeedbackLine);
    releaseSceneRunMutex();
    return false;
  }

  // Listing is fully captured; restore the default handler now so "Done printing file" is
  // recognised during the runScenePath() calls below instead of being silently discarded.
  handshake.setLineHandler(handleMarlinFeedbackLine);

  for (size_t i = 1; i < scenePathCount; ++i)
  {
    int key = scenePathIndices[i];
    char keyExtension[4];
    strncpy(keyExtension, scenePathExtensions[i], sizeof(keyExtension) - 1);
    keyExtension[sizeof(keyExtension) - 1] = '\0';
    size_t j = i;
    while ((j > 0) && (scenePathIndices[j - 1] > key))
    {
      scenePathIndices[j] = scenePathIndices[j - 1];
      strncpy(scenePathExtensions[j], scenePathExtensions[j - 1],
              sizeof(scenePathExtensions[j]) - 1);
      scenePathExtensions[j][sizeof(scenePathExtensions[j]) - 1] = '\0';
      --j;
    }
    scenePathIndices[j] = key;
    strncpy(scenePathExtensions[j], keyExtension, sizeof(scenePathExtensions[j]) - 1);
    scenePathExtensions[j][sizeof(scenePathExtensions[j]) - 1] = '\0';
  }

  for (size_t i = 0; i < scenePathCount; ++i)
  {
    if (!runScenePath(scene, scenePathIndices[i], scenePathExtensions[i]))
    {
      Serial.printf("[runScene] ERROR: failed while running scene %d path %d\n", scene, scenePathIndices[i]);
      releaseSceneRunMutex();
      return false;
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  releaseSceneRunMutex();
  return true;
}

bool sendGCodeFileBlocking(const char* filePath)
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

bool sendGCodeFileListBlocking(const char* listFilePath)
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
    if (!sendGCodeFileBlocking(gcodePath))
    {
      Serial.println("Failed while sending listed file: " + String(gcodePath));
      allSucceeded = false;
    }
  }

  listFile.close();
  Serial.println("Finished G-code list send, files processed: " + String(fileCount));
  return allSucceeded;
}

// Queues a single G-code file to be sent by the background gcodeFileTask; returns whether the
// request was accepted (queued), not whether the transfer has completed. Safe to call from any
// context, including MQTT callbacks, since it never blocks on the actual Marlin transfer.
bool sendGCodeFile(const char* filePath)
{
  if ((filePath == nullptr) || (filePath[0] == '\0'))
  {
    Serial.println("[sendGCodeFile] ERROR: empty file path");
    return false;
  }

  if (gcodeFileRequestQueue == nullptr)
  {
    Serial.println("[sendGCodeFile] ERROR: gcodeFileRequestQueue not initialised");
    return false;
  }

  GCodeFileRequest request;
  request.isList = false;
  strncpy(request.path, filePath, sizeof(request.path) - 1);
  request.path[sizeof(request.path) - 1] = '\0';

  if (xQueueSend(gcodeFileRequestQueue, &request, 0) != pdPASS)
  {
    Serial.printf("[sendGCodeFile] WARNING: queue full, dropped request for '%s'\n", filePath);
    return false;
  }

  return true;
}

// Queues a G-code file list to be sent by the background gcodeFileTask; see sendGCodeFile() for
// the same non-blocking, queue-full-safe semantics.
bool sendGCodeFileList(const char* listFilePath)
{
  if ((listFilePath == nullptr) || (listFilePath[0] == '\0'))
  {
    Serial.println("[sendGCodeFileList] ERROR: empty list file path");
    return false;
  }

  if (gcodeFileRequestQueue == nullptr)
  {
    Serial.println("[sendGCodeFileList] ERROR: gcodeFileRequestQueue not initialised");
    return false;
  }

  GCodeFileRequest request;
  request.isList = true;
  strncpy(request.path, listFilePath, sizeof(request.path) - 1);
  request.path[sizeof(request.path) - 1] = '\0';

  if (xQueueSend(gcodeFileRequestQueue, &request, 0) != pdPASS)
  {
    Serial.printf("[sendGCodeFileList] WARNING: queue full, dropped request for '%s'\n", listFilePath);
    return false;
  }

  return true;
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
  //   1. Streams path RFID0 to home the puck and move the object past the RFID reader.
  //   2. If the RFID reader detected the object, streams path RFID2 to home the puck.
  //      Otherwise, streams path RFID1 to move the object closer to the RFID reader.
  //   3. If an object was detected:
  //        - Resets its pose to the known loaded position (0, 0, heading 90).
  //        - Sends the start-of-day G-code file to move it to its starting position.

  RFIDObjectIndex = -1;
  RFIDEnable = true;    // enable the RFID reader to detect the object

  runSDPathBlocking("System/RFID_0.GCO");
  if (RFIDObjectIndex != -1)
  {
    runSDPathBlocking("System/RFID_1.GCO"); // Move the object past the RFID reader
    localDebug.println("Object detected after first move, sent path to home the puck");
  }
  else
  {
    runSDPathBlocking("System/RFID_2.GCO"); // Run the path to move the object closer to the RFID reader
    localDebug.println("No object detected after first move, sent alternate path to move closer to RFID reader");
  }

  vTaskDelay(1000 / portTICK_PERIOD_MS);

  RFIDEnable = false; // disable the RFID reader now that detection window has closed

  if (RFIDObjectIndex != -1)
  {
    // Object successfully identified — select it and move it to its starting position.
    localDebug.println("Loaded GCode object: " + String(gcodeObjects[RFIDObjectIndex].name));

    char startOfDayFile[32];
    snprintf(startOfDayFile, sizeof(startOfDayFile), "Objects/SoD_%d.GCO", RFIDObjectIndex);
    runSDPathBlocking(startOfDayFile);
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
  snprintf(startOfDayFile, sizeof(startOfDayFile), "Objects/SoD_%d.GCO", index);
  runSDPath(startOfDayFile);
}


