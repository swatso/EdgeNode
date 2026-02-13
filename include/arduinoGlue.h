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

#define GPIO_BANK   0
#define SCENE_BANK  1




//============ Structs, Unions & Enums ============
/*
struct MQTTSensor 
{
    public:
        uint8_t bank;    // Sensor Bank
        uint8_t bitNo;   // Hardware Module bit number (not gpio number)
        int value;       // Current Value
};
*/

//-- from MQTTServices.h
/*
struct MQTTSensor 
{
    public:
        uint8_t bank;    // Sensor Bank
        uint8_t bitNo;   // Hardware Module bit number (not gpio number)
        int value;       // Current Value
};

//-- from MQTTServices.h
struct MQTTDebugPayload
{
    public:
        uint8_t bank;    // Sensor Bank
        char message[BUFFER_SIZE+5];
        boolean appendID = false;
};
*/

//============ Extern Variables ============

//extern char            buffer[20];                        		//-- from FileSystem
//extern int             e;                                 		//-- from FileSystem
//extern String          fileContent;                       		//-- from FileSystem
extern const char*     loudOffsetPath;                    		//-- from FileSystem
extern const char*     nodeConfigPath;                    		//-- from FileSystem
extern const char*     quietOffsetPath;                   		//-- from FileSystem
//extern char*           xx;                                		//-- from FileSystem
//extern QueueHandle_t   MQTTDebugQueue;                    		//-- from MQTTServices
//extern QueueHandle_t   MQTTOperationQueue;                		//-- from MQTTServices
extern void            MQTTPublishSensor(MQTTSensor payload);   //-- from MQTTServices
extern QueueHandle_t   MQTTSensorQueue;                   		//-- from MQTTServices
extern debugStream     debug;                             		//-- from MQTTServices
extern debugStream     operation;                         		//-- from MQTTServices
//extern const char*     PARAM_INPUT_1;                     		//-- from WiFiManager
//extern const char*     PARAM_INPUT_2;                     		//-- from WiFiManager
//extern const char*     PARAM_INPUT_3;                     		//-- from WiFiManager
//extern String          brokerIP;                          		//-- from WiFiManager
//extern boolean         haveConnected;                     		//-- from WiFiManager
//extern const long      interval;                          		//-- from WiFiManager
//extern String          localHostIP;                       		//-- from WiFiManager
//extern String          pass;                              		//-- from WiFiManager
extern int             sceneNo;                           		//-- from WiFiManager
//extern AsyncWebServer  server;                            		//-- from WiFiManager
//extern String          ssid;                              		//-- from WiFiManager
//extern long            wifiConnectionTime;                		//-- from WiFiManager
//extern gpioPin         gpio[];                          		//-- from gpio
//extern byte            nextServo;                         		//-- from gpio
//extern Ticker          servoScanner;                      		//-- from gpio
//extern TaskHandle_t    MQTTDebugService;                  		//-- from gpioServoEasingTest
//extern TaskHandle_t    MQTTSensorService;                 		//-- from gpioServoEasingTest
//extern String          Version;                           		//-- from gpioServoEasingTest
//extern long            exitTime;                          		//-- from gpioServoEasingTest

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
//-- from gpio.ino -----------
//void setupGPIO();                                           
//void powerGPIO();                                           
                       
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
void setupMQTTServices();                                   
void MQTTPublishSensor(MQTTSensor payload);                 
void MQTTPublishDebug(MQTTDebugPayload payload);            
//-- from WiFiManager.ino -----------
void playAudio(int x, bool y);                              
void stopAudio();                                           
bool initWiFi();                                            
void checkWiFiConnection();                                 
void setupWiFi();                                           
//unsigned long WiFiGetCommsUptime();                             
//String WiFiGetIPAddress();                                      
//String WiFiGetSSID();                                           
//int WiFietRSSI();                                              






#endif // ARDUINOGLUE_H
