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
int action7PlayFcn(uint8_t number);
int action7StopFcn(uint8_t number);
int action8PlayFcn(uint8_t number);
int action8StopFcn(uint8_t number);
int templatePlayFcn(uint8_t number);
int templateStopFcn(uint8_t number);
int runSwitchHandler(uint8_t number);
bool scene2RunFcn(uint8_t number);
bool scene3RunFcn(uint8_t number);
bool scene8RunFcn(uint8_t number);

void scene2CompletionHandler(uint8_t sceneIndex, bool success);
void scene3CompletionHandler(uint8_t sceneIndex, bool success);
void scene4CompletionHandler(uint8_t sceneIndex, bool success);
void scene5CompletionHandler(uint8_t sceneIndex, bool success);
void scene6CompletionHandler(uint8_t sceneIndex, bool success);
void scene7CompletionHandler(uint8_t sceneIndex, bool success);

#endif // USERCODE_H
