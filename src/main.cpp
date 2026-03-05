                              
#include "gpioServoEasingTest.h"
#include "MQTTComms.h"                 
#include "gpio.h"
#include "sound.h"
#include "action.h"
#include "system.h"
#include "debugStream.h"
#include "UserCode.h"
#include "main.h"


String Version = "GPIO Test 05/Feb/26";

//#define debugTest

void setup()
{
  Serial.begin(115200);           // for debug only
  powerGPIO(false);
  delay(2000);
  setupSPIFFS();
  node.loadConfig();
  setupUserCode();
  mp3.loadConfig();
  setupAction();
  loadActionConfig();
  setupGPIO();
  loadServoPositions();
  setupWiFi();
  setupMQTTPaths();
  setupMQTTServices();
  setupMQTTComms();
  setupSound();
  powerGPIO(true);

Serial.print("Core:");
Serial.println(xPortGetCoreID());

//debug.println("Completed Setup");
}

void loop()
{
  pause(5000);
getSystemInfo();
pause(60000);
}

long exitTime;
void pause(long mS)
{
  exitTime = millis() + mS;
  while (millis() < exitTime)
  {
    // background stuff
    yield();
  }
  return;
}

void setupMQTTPaths()
{
  // This function sets the MQTT topic paths for the debug and operations streams
  localDebug.setNodeID(node.getNodeID());
  localDebug.appendNodeID(false);
  localOperations.setNodeID(node.getNodeID());
  localOperations.appendNodeID(false);
  globalDebug.setNodeID(node.getNodeID());
  globalOperations.setNodeID(node.getNodeID());
  globalDebug.appendNodeID(true);
  globalOperations.appendNodeID(true);
}