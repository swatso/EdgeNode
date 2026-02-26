                              
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
  mp3.loadConfig();
  loadActionConfig();
  setupUserCode();
  setupGPIO();
  loadServoPositions();
  setupWiFi();
  setupMQTTServices();
  setupMQTTComms();
  setupSound();
  setupAction();

  // configure the debug streams with nodeID and whether to append the nodeID to the topic or not (for global topics)
  localDebug.setNodeID(node.getNodeID());
  localDebug.appendNodeID(false);
  localOperations.setNodeID(node.getNodeID());
  localOperations.appendNodeID(false);
  globalDebug.setNodeID(node.getNodeID());
  globalOperations.setNodeID(node.getNodeID());
  globalDebug.appendNodeID(true);
  globalOperations.appendNodeID(true);

  powerGPIO(true);

Serial.print("Core:");
Serial.println(xPortGetCoreID());

//debug.println("Completed Setup");
}

void loop()
{
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