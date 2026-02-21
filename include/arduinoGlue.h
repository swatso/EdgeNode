#ifndef ARDUINOGLUE_H
#define ARDUINOGLUE_H


//============ Includes ====================
#include <SPIFFS.h>
#include <Ticker.h>
#include <Arduino.h>
#include <cstring>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP32Servo.h>                         
#include "ServoEasing.h"

#include "gpio.h"
//#include "NodeServices.h"
#include "MQTTServices.h"

//============ Defines & Macros====================
#define HASH 35
#define POWER_GPIO 2




//============ Extern Variables ============


extern const char*     loudOffsetPath;                    		//-- from FileSystem
extern const char*     nodeConfigPath;                    		//-- from FileSystem
extern const char*     quietOffsetPath;                   		//-- from FileSystem
//extern QueueHandle_t   MQTTDebugQueue;                    		//-- from MQTTServices
//extern QueueHandle_t   MQTTOperationQueue;                		//-- from MQTTServices
extern void            MQTTPublishSensor(MQTTSensor payload);   //-- from MQTTServices
extern QueueHandle_t   MQTTSensorQueue;                   		//-- from MQTTServices
//extern debugStream     debug;                             		//-- from MQTTServices
//extern debugStream     operation;                         		//-- from MQTTServices
//extern int             sceneNo;                           		//-- from WiFiManager


//============ Function Prototypes =========
//-- from FileSystem.ino -----------
void setupSPIFFS();                                         
String readFile(fs::FS &fs, const char *path, const char *deft);
char* readFileC(fs::FS &fs, const char *path, const char *deft);
void writeFile(fs::FS &fs, const char * path, const char * message);
void writeFile(fs::FS &fs, const char * path, int value);
void writeConfigFile(fs::FS &fs, uint8_t bit);              
void readConfigFile(fs::FS &fs, uint8_t bit);               
void deleteFile(fs::FS &fs, const char * path);             
                                         
                       
//-- from gpioServoEasingTest.ino -----------
void setupApplication();                                    
void sensorReceiverTask(void *pvParameters);                
void debugReceiverTask(void *pvParameters);                 
void testPWMLocalReadWrite();                               
void testPWMRemoteReadWrite();                              
void testDOUTLocalReadWrite();                              
void testDOUTRemoteReadWrite();                             
void testDINLocalRead();                                    
void testDINRemoteRead();                                   
void testAINLocalRead();                                    
void testAINRemoteRead();                                   
void testServoLocalReadWrite();                             
void testServoRemoteReadWrite();                            
void multiServoTest();                                      
void pause(long mS);                                        
//-- from MQTTServices.ino -----------
//void setupMQTTServices();                                   
//void MQTTPublishSensor(MQTTSensor payload);                 
//void MQTTPublishDebug(MQTTDebugPayload payload);            
//-- from WiFiManager.ino -----------
void playAudio(int x, bool y);                              
void stopAudio();                                           
bool initWiFi();                                            
void checkWiFiConnection();                                 
void setupWiFi();                                           
                                          

#endif // ARDUINOGLUE_H
