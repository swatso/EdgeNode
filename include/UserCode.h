#ifndef USERCODE_H
#define USERCODE_H

#include <Arduino.h>
#include "action.h"

void setupUserCode();
bool sendGCodeFile(const char* filePath);
bool sendGCodeFileList(const char* listFilePath);
int action1PlayFcn(uint8_t number);
int action1StopFcn(uint8_t number);
int action2PlayFcn(uint8_t number);
int action2StopFcn(uint8_t number);
int action3PlayFcn(uint8_t number);
int action3StopFcn(uint8_t number);
int templatePlayFcn(uint8_t number);
int templateStopFcn(uint8_t number);
int runSwitchHandler(uint8_t number);

#endif // USERCODE_H
