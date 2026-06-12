#ifndef USERCODE_H
#define USERCODE_H

#include <Arduino.h>
#include "action.h"
#include "GCodeControl.h"

void setupUserCode();
int action1PlayFcn(uint8_t number);
int action1StopFcn(uint8_t number);
int action2PlayFcn(uint8_t number);
int action2StopFcn(uint8_t number);
int action3PlayFcn(uint8_t number);
int action3StopFcn(uint8_t number);
int action4PlayFcn(uint8_t number);
int action4StopFcn(uint8_t number);
int action5PlayFcn(uint8_t number);
int action5StopFcn(uint8_t number);
int action6PlayFcn(uint8_t number);
int action6StopFcn(uint8_t number);
int templatePlayFcn(uint8_t number);
int templateStopFcn(uint8_t number);
int runSwitchHandler(uint8_t number);

#endif // USERCODE_H
