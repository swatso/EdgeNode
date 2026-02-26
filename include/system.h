#ifndef SYSTEM_H
#define SYSTEM_H

#include <Arduino.h>
#include <stdio.h>
#include "gpio.h"

void setupSystem();
void getSystemInfo();
int vCheckStackFreeSpace(TaskHandle_t xHandle);

#endif // SYSTEM_H