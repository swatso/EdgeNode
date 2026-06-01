#include "GCodeControl.h"

#include "debugStream.h"
#include "marlin_handshake.h"

#include <SPIFFS.h>
#include <cstring>

namespace {
HardwareSerial gcodeUart(1);  // UART1 (use 1 or 2 typically)
MarlinHandshake<> handshake(gcodeUart);
}

void initGCodeControl(uint32_t baud, int8_t rxPin, int8_t txPin)
{
  gcodeUart.begin(baud, SERIAL_8N1, rxPin, txPin);
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