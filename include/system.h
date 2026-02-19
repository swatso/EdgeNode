#ifndef SYSTEM_H
#define SYSTEM_H

//#include <FreeRTOS.h>
//#include <task.h>
#include <Arduino.h>
#include <stdio.h>

#include "gpio.h"



void setupSystem();
void getSystemInfo();
int vCheckStackFreeSpace(TaskHandle_t xHandle);










#endif // SYSTEM_H