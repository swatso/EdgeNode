#include "GCodeControl.h"

#include "debugStream.h"
#include "marlin_handshake.h"

#include <SPIFFS.h>
#include <cstring>

namespace {
HardwareSerial gcodeUart(1);  // UART1 (use 1 or 2 typically)
MarlinHandshake<> handshake(gcodeUart);

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
}

GCodeObject gcodeObjects[16];
int objectLoading = -1; // -1 means no object is currently being loaded, otherwise holds the index of the object being loaded

void initGCodeControl(uint32_t baud, int8_t rxPin, int8_t txPin)
{
  gcodeUart.begin(baud, SERIAL_8N1, rxPin, txPin);
  gcodeObjects[0].setName("Tarmac Layer");
  gcodeObjects[0].setRFIDTag("C6CB4A92");
  gcodeObjects[1].setName("JCB 3CX");
  gcodeObjects[1].setRFIDTag("RFID_1");
  gcodeObjects[2].setName("Roller");
  gcodeObjects[2].setRFIDTag("RFID_2");
  gcodeObjects[3].setName("Workmen Pipe");
  gcodeObjects[3].setRFIDTag("C6CB4A7B");

}

void MarlinSender(const char* line) {
  while (!handshake.canSendNow()) {
    // Wait until it's safe to send the next command
    vTaskDelay(100 / portTICK_PERIOD_MS);
    handshake.processInput(); // Process any incoming responses from Marlin
  }
  handshake.sendLine(line);
  Serial.println("Sent G-code to Marlin:");
  Serial.println(line);
  vTaskDelay(100 / portTICK_PERIOD_MS);
}

bool sendGCodeFile(const char* filePath)
{
  if ((filePath == nullptr) || (filePath[0] == '\0'))
  {
    localDebug.println("G-code file path is empty");
    return false;
  }

  File gcodeFile = SPIFFS.open(filePath, FILE_READ);
  if (!gcodeFile || gcodeFile.isDirectory())
  {
    localDebug.println("Failed to open G-code file: " + String(filePath));
    return false;
  }

  localDebug.println("Sending G-code from file: " + String(filePath));

  char lineBuffer[128];
  size_t sentLines = 0;

  while (gcodeFile.available())
  {
    size_t len = gcodeFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
    lineBuffer[len] = '\0';

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
      continue;
    }

    if (line[0] == ';')
    {
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
        continue;
      }
    }

    MarlinSender(line);
    ++sentLines;
  }

  gcodeFile.close();
  localDebug.println("Finished G-code file send, lines sent: " + String(sentLines));
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
    char lastPoseLine[128] = "";
    float x = 0, y = 0, bearing = 0;
    while (gcodeFile.available())
    {
        size_t len = gcodeFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
        lineBuffer[len] = '\0';

        if (strncmp(lineBuffer, "G1", 2) == 0)
        {
            strncpy(lastPoseLine, lineBuffer, sizeof(lastPoseLine) - 1);
            // Line is of the form G1 X100 Y100 Z90 F800
            // We will extract X, Y and Z values from this line

            char* token = strtok(lineBuffer, " ");
            while (token != nullptr)
            {
                if (token[0] == 'X')
                {
                    x = atof(&token[1]);
                }
                else if (token[0] == 'Y')
                {
                    y = atof(&token[1]);
                }
                else if (token[0] == 'Z')
                {
                    bearing = atof(&token[1]);
                }
                token = strtok(nullptr, " ");
            }
        }
        // now update the pose of the object being loaded with the last pose found in the file
        if (lastPoseLine[0] != '\0')
        {
            gcodeObjects[objectIndex].loadPose(x, y, bearing);
            localDebug.println("Updated pose for object " + String(gcodeObjects[objectIndex].name) + " to (" + String(x) + ", " + String(y) + ") bearing " + String(bearing));
        }
    }
    gcodeFile.close();
}


void GCodeObjectRFIDReporter(const char* rfidTag)
{
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
      snprintf(message, sizeof(message), "Matched GCode object: %s at (%.2f, %.2f) bearing %.2f", gcodeObjects[i].name, gcodeObjects[i].x, gcodeObjects[i].y, gcodeObjects[i].bearing);
      localDebug.println(message);
      objectLoading = i;
      break;
    }
  }
}

void loadGCodeObject()
{
    // This function is invoked by the user (typically via an MQTT event or by pressing a pushbutton)
    // It performs th following actions:-
    //      It streams file pathRFID1 to home the puck and move the object past the RFID reader
    //      If the reader has detected the object then it streams path pathRFID3 to home the puck
    //      If not then it streams the pathRFID2 which move the object closer to the RFID  reader 
    //  After this is done, if an object has been detected then its Pose is updated

  objectLoading = -1; // reset to indicate no object is currently being loaded
  sendGCodeFile("/pathRFID1.gcode");
  Serial.println("XXXXX");
  vTaskDelay(30000 / portTICK_PERIOD_MS);
  Serial.println("YYYYY");
  if(objectLoading != -1)
  {
    sendGCodeFile("/pathRFID3.gcode");      // Tag has been detected, send the path to home the puck
    localDebug.println("Object detected after first move, sent path to home the puck");
  }
  else
  {
    sendGCodeFile("/pathRFID2.gcode");
    localDebug.println("No object detected after first move, sent alternate path to move closer to RFID reader");
  }
  if(objectLoading != -1)
  {
    gcodeObjects[objectLoading].loadPose(0, 0, 90);
    localDebug.println("Loaded GCode object: " + String(gcodeObjects[objectLoading].name));

    // Now send the Start Of Day file for the object to move it to its starting position
    char startOfDayFile[32];
    snprintf(startOfDayFile, sizeof(startOfDayFile), "/startOfDay%d.gcode", objectLoading);
    sendGCodeFile(startOfDayFile);
    updatePoseFromFile(startOfDayFile, objectLoading);
  }
}

