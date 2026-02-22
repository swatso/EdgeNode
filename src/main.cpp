//#include <Ticker.h>                               		//-- moved to arduinoGlue.h
#include "gpioServoEasingTest.h"
//#include <Arduino.h>
#include "MQTTComms.h"                              		//-- moved to arduinoGlue.h
#include "MQTTServices.h"
#include "nodeServices.h"
#include "gpio.h"
#include "sound.h"
#include "action.h"
#include "arduinoGlue.h"
#include "system.h"
#include "debugStream.h"

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
  setupApplication();
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


void  setupApplication()
{
//gpio[0].setType(GPIO_PWM);
 //gpio[1].setType(GPIO_DIGOUT);
 //gpio[2].setType(GPIO_SERVO);
 //gpio[3].setType(GPIO_DIGIN);
 //gpio[5].setType(GPIO_AIN);

//gpio[0].setType(GPIO_DIGIN);
//gpio[0].rate = 1000;
//gpio[0].preset0 = 10000;

//gpio[0].setType(GPIO_AIN);
//gpio[0].rate = 500;
//gpio[0].preset0 = 10000;
//gpio[0].preset1 = 0;   // deadband

//gpio[0].setType(GPIO_DIN);
//gpio[0].rate = 500;
//gpio[0].preset0 = 10000;

/*
gpio[0].setType(GPIO_SERVO);
//gpio[0].setEasingType(EASE_CUBIC_IN_OUT);
gpio[1].setType(GPIO_SERVO);
gpio[2].setType(GPIO_SERVO);
gpio[3].setType(GPIO_SERVO);
gpio[4].setType(GPIO_SERVO);
gpio[5].setType(GPIO_SERVO);
gpio[6].setType(GPIO_SERVO);
gpio[7].setType(GPIO_SERVO);
gpio[8].setType(GPIO_SERVO);
gpio[9].setType(GPIO_SERVO);
gpio[10].setType(GPIO_SERVO);
gpio[11].setType(GPIO_SERVO);
gpio[12].setType(GPIO_SERVO);
gpio[13].setType(GPIO_SERVO);
//gpio[14].setType(GPIO_SERVO);
//gpio[15].setType(GPIO_SERVO);

*/



}



void loop()
{
//  testPWMLocalReadWrite();
//  testPWMRemoteReadWrite();
//    testDOUTLocalReadWrite();
//    testDOUTRemoteReadWrite();
//    testDINLocalRead();
//    testDINRemoteRead();
//    testAINLocalRead();
//    testAINRemoteRead();
//    testServoLocalReadWrite();
//    testServoRemoteReadWrite();
//      multiServoTest();
//gpio[0].preset1 = 128;
//gpio[0].preset2 = 255;
//gpio[0].write(true);
//pause(15000);
//gpio[0].write(false);
getSystemInfo();
pause(60000);
}


void testPWMLocalReadWrite()
{
  //test PWM
    //gpio[0].enableLocal = false;
    gpio[0].localWrite(0);
    Serial.print("PWM localWrite demand:0  localRead:");
    Serial.println(gpio[0].localRead());
    delay(1000);
    gpio[0].localWrite(128);
    Serial.print("PWM localWrite demand:128  localRead:");
    Serial.println(gpio[0].localRead());
    delay(1000);
    gpio[0].localWrite(255);
    Serial.print("PWM localWrite demand:255  localRead:");
    Serial.println(gpio[0].localRead());
    delay(1000);
}

void testPWMRemoteReadWrite()
{
  //test PWM
    //gpio[0].enableRemote = false;
    gpio[0].remoteWrite(0);
    Serial.print("PWM remoteWrite demand:0  remoteRead:");
    Serial.println(gpio[0].remoteRead());
    delay(1000);
    gpio[0].remoteWrite(128);
    Serial.print("PWM remoteWrite demand:128  remoteRead:");
    Serial.println(gpio[0].remoteRead());
    delay(1000);
    gpio[0].remoteWrite(255);
    Serial.print("PWM remoteWrite demand:255  remoteRead:");
    Serial.println(gpio[0].remoteRead());
    delay(1000);
}

void testDOUTLocalReadWrite()
{
  //test DOUT
  //  gpio[1].enableLocal = false;
    gpio[1].localWrite(0);
    Serial.print("DOUT localWrite demand:0  localRead:");
    Serial.println(gpio[1].localRead());
    delay(1000);
    gpio[1].localWrite(1);
    Serial.print("DOUT localWrite demand:1  localRead:");
    Serial.println(gpio[1].localRead());
    delay(1000);
    gpio[1].localWrite(2);
    Serial.print("DOUT localWrite demand:2  localRead:");
    Serial.println(gpio[1].localRead());
    delay(1000);
    gpio[1].localWrite(255);
    Serial.print("DOUT localWrite demand:255  localRead:");
    Serial.println(gpio[1].localRead());
    delay(1000);
}

void testDOUTRemoteReadWrite()
{
  //test DOUT
//    gpio[1].enableRemote = false;
    gpio[1].remoteWrite(0);
    Serial.print("DOUT remoteWrite demand:0  remoteRead:");
    Serial.println(gpio[1].remoteRead());
    delay(1000);
    gpio[1].remoteWrite(1);
    Serial.print("DOUT remoteWrite demand:1  remoteRead:");
    Serial.println(gpio[1].remoteRead());
    delay(1000);
    gpio[1].remoteWrite(2);
    Serial.print("DOUT remoteWrite demand:2  remoteRead:");
    Serial.println(gpio[1].remoteRead());
    delay(1000);
}

void  testDINLocalRead()
{
  //gpio[3].enableLocal = false;
  Serial.print("DIN LocalRead :");
  Serial.println(gpio[3].localRead());
  delay(1000);
}

void  testDINRemoteRead()
{
  gpio[3].enableRemote = false;
  Serial.print("DIN RemoteRead :");
  Serial.println(gpio[3].remoteRead());
  delay(1000);
}

void  testAINLocalRead()
{
  gpio[5].enableLocal = false;
  Serial.print("AIN LocalRead :");
  Serial.println(gpio[5].localRead());
  delay(1000);
}

void  testAINRemoteRead()
{
  gpio[5].enableRemote = false;
  Serial.print("AIN RemoteRead :");
  Serial.println(gpio[5].remoteRead());
  delay(1000);
}

void testServoLocalReadWrite()
{
  //test Servo
    //gpio[2].enableLocal = false;
    //gpio[2].localWrite(0);
    Serial.print("Servo localWrite demand:0  localRead:");
    Serial.println(gpio[2].localRead());
    pause(10000);
    Serial.println(gpio[2].localRead());
    pause(10000);
     Serial.println(gpio[2].localRead());
    pause(10000);
    Serial.println(gpio[2].localRead());
    pause(10000);
    Serial.println(gpio[2].localRead());
    pause(10000);
    Serial.println(gpio[2].localRead());
    pause(10000);
    Serial.println(gpio[2].localRead());
    pause(10000);
    Serial.println(gpio[2].localRead());
    pause(10000);
    Serial.println(gpio[2].localRead());
    pause(10000);
    Serial.println(gpio[2].localRead());
    pause(10000);
    Serial.println(gpio[2].localRead());

    gpio[2].localWrite(90);
    Serial.print("Servo localWrite demand:90  localRead:");
    Serial.println(gpio[2].localRead());
    pause(50000);
    Serial.println(gpio[2].localRead());
    
    gpio[2].localWrite(180);
    Serial.print("Servo localWrite demand:180  localRead:");
    Serial.println(gpio[2].localRead());
    pause(200000);
    Serial.println(gpio[2].localRead());
}

void testServoRemoteReadWrite()
{
  //test Servo
    //gpio[2].enableRemote = false;
    gpio[2].remoteWrite(0);
    Serial.print("Servo remoteWrite demand:0  localRead:");
    Serial.println(gpio[2].remoteRead());
    delay(10000);
    Serial.println(gpio[2].remoteRead());
    gpio[2].remoteWrite(90);
    Serial.print("Servo remoteWrite demand:128  localRead:");
    Serial.println(gpio[2].remoteRead());
    delay(5000);
    Serial.println(gpio[2].remoteRead());
    gpio[2].remoteWrite(180);
    Serial.print("Servo remoteWrite demand:180  localRead:");
    Serial.println(gpio[2].remoteRead());
    delay(10000);
    Serial.println(gpio[2].remoteRead());
}

void multiServoTest()
{
  Serial.println("0");
  gpio[0].remoteWrite(0);
  gpio[1].remoteWrite(0);
  gpio[2].remoteWrite(0);
  gpio[3].remoteWrite(0);
  gpio[4].remoteWrite(0);
  gpio[5].remoteWrite(0);
  gpio[6].remoteWrite(0);
  gpio[7].remoteWrite(0);
  gpio[8].remoteWrite(0);
  gpio[9].remoteWrite(0);
  gpio[10].remoteWrite(0);
  gpio[11].remoteWrite(0);
  gpio[12].remoteWrite(0);
  gpio[13].remoteWrite(0);
  gpio[14].remoteWrite(0);
  gpio[15].remoteWrite(0);
  delay(10000);

  Serial.println("90");
  gpio[0].remoteWrite(90);
  gpio[1].remoteWrite(90);
  gpio[2].remoteWrite(90);
  gpio[3].remoteWrite(90);
  gpio[4].remoteWrite(90);
  gpio[5].remoteWrite(90);
  gpio[6].remoteWrite(90);
  gpio[7].remoteWrite(90);
  gpio[8].remoteWrite(90);
  gpio[9].remoteWrite(90);
  gpio[10].remoteWrite(90);
  gpio[11].remoteWrite(90);
  gpio[12].remoteWrite(90);
  gpio[13].remoteWrite(90);
  gpio[14].remoteWrite(90);
  gpio[15].remoteWrite(90);
  delay(10000);

  Serial.println("180");
  gpio[0].remoteWrite(180);
  gpio[1].remoteWrite(180);
  gpio[2].remoteWrite(180);
  gpio[3].remoteWrite(180);
  gpio[4].remoteWrite(180);
  gpio[5].remoteWrite(180);
  gpio[6].remoteWrite(180);
  gpio[7].remoteWrite(180);
  gpio[8].remoteWrite(180);
  gpio[9].remoteWrite(180);
  gpio[10].remoteWrite(180);
  gpio[11].remoteWrite(180);
  gpio[12].remoteWrite(180);
  gpio[13].remoteWrite(180);
  gpio[14].remoteWrite(180);
  gpio[15].remoteWrite(180);
  delay(10000);


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