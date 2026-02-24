#include "system.h"
#include <Arduino.h>
#include "gpio.h"
#include "action.h"

void setupSystem()
{
    Serial.println("Setting up system...");
}

void getSystemInfo()
{
    Serial.print("Free Heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.print("Sketch Size: ");
    Serial.println(ESP.getSketchSize());
    Serial.print("Sketch Space: ");
    Serial.println(ESP.getFreeSketchSpace());


    Serial.println();
    Serial.println("GPIO Helper Tasks Stack Free Space:");
    vCheckStackFreeSpace(gpioTask0);
    vCheckStackFreeSpace(gpioTask1);
    vCheckStackFreeSpace(gpioTask2);
    vCheckStackFreeSpace(gpioTask3);
    vCheckStackFreeSpace(gpioTask4);
    vCheckStackFreeSpace(gpioTask5);
    vCheckStackFreeSpace(gpioTask6);
    vCheckStackFreeSpace(gpioTask7);
    vCheckStackFreeSpace(gpioTask8);
    vCheckStackFreeSpace(gpioTask9);
    vCheckStackFreeSpace(gpioTask10);
    vCheckStackFreeSpace(gpioTask11);
    vCheckStackFreeSpace(gpioTask12);
    vCheckStackFreeSpace(gpioTask13);
    vCheckStackFreeSpace(gpioTask14);
    vCheckStackFreeSpace(gpioTask15);
    vCheckStackFreeSpace(servoSaverTask);
    vCheckStackFreeSpace(actionTask0);
    vCheckStackFreeSpace(actionTask1);
    vCheckStackFreeSpace(actionTask2);
    vCheckStackFreeSpace(actionTask3);
    vCheckStackFreeSpace(actionTask4);
    vCheckStackFreeSpace(actionTask5);
    vCheckStackFreeSpace(actionTask6);
    vCheckStackFreeSpace(actionTask7);
    vCheckStackFreeSpace(actionTask8);
    vCheckStackFreeSpace(actionTask9);
    vCheckStackFreeSpace(actionTask10);
    vCheckStackFreeSpace(actionTask11);
    vCheckStackFreeSpace(actionTask12);
    vCheckStackFreeSpace(actionTask13);
    vCheckStackFreeSpace(actionTask14);
    vCheckStackFreeSpace(actionTask15);
}


int vCheckStackFreeSpace(TaskHandle_t xHandle)
{
    // returns the number of unused stack elements for the specified task
        UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(xHandle);
        Serial.print("Task: ");
        Serial.print(pcTaskGetName(xHandle));
        Serial.print(" Stack High Water Mark: ");
        Serial.println(highWaterMark);
        return highWaterMark;
}

