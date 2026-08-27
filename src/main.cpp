                              
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
  delay(5000);
  setupSPIFFS();
  node.loadConfig();
  setupMQTTPaths();
  setupWiFi();
  setupMQTTServices();
  setupMQTTComms();
  setupUserCode();
  mp3.loadConfig();
  //setupAction();
  //loadActionConfig();
  setupGPIO();
  loadServoPositions();
  setupSound();
  powerGPIO(true);

Serial.print("Core:");
Serial.println(xPortGetCoreID());

  // TEMPORARY DIAGNOSTIC: sends a fixed sequence of simple G-code commands and logs raw
  // send/ack timing, to help isolate Marlin handshake lockups seen at power-up. Remove
  // (or comment out) once the lockup is diagnosed/fixed.
 // runMarlinHandshakeSelfTest();

 //Serial.println("Starting SD Path");
 //runSDPath("OBJ_8/PATH_0.GCO"); // test path to run on startup, for testing only
 //Serial.println("Completed SD Path");

 //Serial.println("Starting SD Path Blocking");
 //runSDPathBlocking("OBJ_8/PATH_0.GCO"); // test path to run on startup, for testing only
 //Serial.println("Completed SD Path Blocking");

//debug.println("Completed Setup");
}

void loop()
{
  pause(5000);
//  getSystemInfo();
//pause(60000);
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

  //runSDPath("OBJ_8/PATH_0.GCO"); // test path to run on startup, for testing only
}