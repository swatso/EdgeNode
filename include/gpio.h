#ifndef GPIO_H
#define GPIO_H

//============ Includes ====================
//#include "arduinoGlue.h"
#include <ESP32Servo.h>                         
#include "ServoEasing.h"

//const char* enableLocalPath = "/enableLocal#.txt";
//const char* enableRemotePath = "/enableRemote#.txt";

void setupGPIO();                                           
void powerGPIO();  

// define GPIO PIN mappings
// These correspond to the PCB design assignments
#define GPIO_BIT00 13
#define GPIO_BIT01 12
#define GPIO_BIT02 4
#define GPIO_BIT03 14
#define GPIO_BIT04 16
#define GPIO_BIT05 27
#define GPIO_BIT06 17
#define GPIO_BIT07 26
#define GPIO_BIT08 25
#define GPIO_BIT09 18
#define GPIO_BIT0A 19
#define GPIO_BIT0B 32
#define GPIO_BIT0C 21
#define GPIO_BIT0D 22
#define GPIO_BIT0E 23
#define GPIO_BIT0F 33

// define I/O types
#define GPIO_NONE   0
#define GPIO_PWM    1
#define GPIO_DIGOUT 2
#define GPIO_SERVO  3
#define GPIO_DIGIN  4
#define GPIO_AIN 5
#define GPIO_DIGOUT_PULSE 6
#define GPIO_PWM_PULSE 7

// GPIO Controller Object
// Encapsulates the data and methods for GPIO Control
class gpioPin
{
  public:
    char name[20];            // Friendly name
    uint8_t pin;              // GPIO hardware pin
    uint8_t bitNo;            // PCB bit number
    uint8_t type;             // Function type of this GPIO pin (see I/O types)
    int preset0;
    int preset1;
    int preset2;
    int rate;                 // Polling rate for inputs (mS), slew rate for servos (deg/S)
    ServoEasing*  servo;      // pointer to low level servo control object (if type = GPIO_SERVO)
    boolean enableLocal;
    boolean enableRemote;

    gpioPin();         // constructor
    int getValue();
    void setType(uint8_t newType);
    void alwaysWrite(int demand);
    void localWrite(int demand);
    void remoteWrite(int demand);
    int localRead();
    int remoteRead();
    void setPublishRate(int publishRate_ms);         // background periodic publish via MQTT (even if GPIO has not changed)
    int getPublishRate();
    void setEasingType(uint_fast8_t aEasingType);
    int getEasingType();
    void helper();                  // This is essentially private to the class but is called by the external helper task
                                    // FreeRtos does not allow this function to be instantiated as a task directly

private:
    int value;
    int target;                     // Servo
    uint_fast8_t easingType;        // Servo
    int publishRate;
    long nextPublish;               // stores time for next background publish for helper task
    void write(int demand);
    int read();
    void servoController();
    void inputPoller();
    void outputPulser();
    void publish(uint8_t bit, int aValue);
    SemaphoreHandle_t lock;
};

void helper0(void * pvParameters);                          
void helper1(void * pvParameters);                          
void helper2(void * pvParameters);                          
void helper3(void * pvParameters);                          
void helper4(void * pvParameters);                          
void helper5(void * pvParameters);                          
void helper6(void * pvParameters);                          
void helper7(void * pvParameters);                          
void helper8(void * pvParameters);                          
void helper9(void * pvParameters);                          
void helper10(void * pvParameters);                         
void helper11(void * pvParameters);                         
void helper12(void * pvParameters);                         
void helper13(void * pvParameters);                         
void helper14(void * pvParameters);                         
void helper15(void * pvParameters); 

extern gpioPin         gpio[]; 

#endif //GPIO_H
