#ifndef GCODECONTROL_H
#define GCODECONTROL_H

#include <Arduino.h>

void initGCodeControl(uint32_t baud = 250000, int8_t rxPin = 22, int8_t txPin = 23);
void MarlinSender(const char* line);
bool sendGCodeFile(const char* filePath);
bool sendGCodeFileList(const char* listFilePath);

#endif // GCODECONTROL_H