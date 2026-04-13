#ifndef GPIO_H
#define GPIO_H

//============ Includes ====================
//#include "arduinoGlue.h"
#include <ESP32Servo.h>                         
#include "ServoEasing.h"

//const char* enableLocalPath = "/enableLocal#.txt";
//const char* enableRemotePath = "/enableRemote#.txt";

void setupGPIO();
void servoSaver();
void loadServoPositions();                                           
void powerGPIO(bool powerUp);
bool GPIOstate();  

#define SERVO_SETTLING_DELAY 5000   // delay (mS) after writing a new position to allow the servo to settle before saving the position to SPIFFS

// ---------------------------------------------------------------------------
// GPIO PIN mappings — selected by the build-target macro set in platformio.ini
// ---------------------------------------------------------------------------

#if defined(HW_ESP32DEV)
// Original PCB hardware — ESP32 Dev Kit pin assignments
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
#define RUN1       36
#define RUN2       39
#define POWER_GPIO 2

#elif defined(HW_ESP32C3)
// Placeholder — future ESP32-C3 based PCB hardware pin assignments
// Update these values to match the actual PCB design when finalised.
// Note: ESP32-C3 has fewer GPIOs than the standard ESP32; GPIO 11-17 are
// typically reserved for internal flash on devkit modules. Pins below marked
// "TBD" will need reassigning once the PCB schematic is available.
#define GPIO_BIT00 0
#define GPIO_BIT01 1
#define GPIO_BIT02 2
#define GPIO_BIT03 3
#define GPIO_BIT04 4
#define GPIO_BIT05 5
#define GPIO_BIT06 6
#define GPIO_BIT07 7
#define GPIO_BIT08 8
#define GPIO_BIT09 9
#define GPIO_BIT0A 10
#define GPIO_BIT0B 18
#define GPIO_BIT0C 19
#define GPIO_BIT0D 20
#define GPIO_BIT0E 21
#define GPIO_BIT0F 11  // TBD — reassign when PCB design is finalised
#define RUN1       12  // TBD — reassign when PCB design is finalised
#define RUN2       13  // TBD — reassign when PCB design is finalised
#define POWER_GPIO 14  // TBD — reassign when PCB design is finalised

#else
#error "No hardware target defined. Set HW_ESP32DEV or HW_ESP32C3 in platformio.ini build_flags."
#endif

// define I/O types
#define GPIO_NONE   0
#define GPIO_PWM    1
#define GPIO_DIGOUT 2
#define GPIO_SERVO 3
#define GPIO_SERVO_ACTUATOR 4
#define GPIO_DIGIN  5
#define GPIO_AIN 6
#define GPIO_DIGOUT_PULSE 7
#define GPIO_PWM_PULSE 8


void queServoPosition(uint8_t bitNo, int position, unsigned int delay);

// Encapsulates the data for saving the position single servo
struct servoData
{
  uint8_t bitNo;                       // PCB bit number
  int position;                       // current position of the servo
  unsigned long timestamp;            // time of the last update + a delay to settle
};

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
    void initValue(int initValue); // This is called after the GPIO type has been set to initialise the value of the GPIO (eg to set the initial position of a servo)  
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
    void outputHelper();
    void outputPulser();
    void publish(uint8_t bit, int aValue);
    SemaphoreHandle_t lock;
};

// Queue handles
extern QueueHandle_t servoQueue;

// Task handles for GPIO helper tasks - exposed so system monitoring can check stack free space
extern TaskHandle_t gpioTask0;
extern TaskHandle_t gpioTask1;
extern TaskHandle_t gpioTask2;
extern TaskHandle_t gpioTask3;
extern TaskHandle_t gpioTask4;
extern TaskHandle_t gpioTask5;
extern TaskHandle_t gpioTask6;
extern TaskHandle_t gpioTask7;
extern TaskHandle_t gpioTask8;
extern TaskHandle_t gpioTask9;
extern TaskHandle_t gpioTask10;
extern TaskHandle_t gpioTask11;
extern TaskHandle_t gpioTask12;
extern TaskHandle_t gpioTask13;
extern TaskHandle_t gpioTask14;
extern TaskHandle_t gpioTask15;
extern TaskHandle_t servoSaverTask;

// Corresponding helper function
void gpioHelperTask(void * pvParameters);
void servoSaver(void * pvParameters);

bool run1Switch();
bool run2Switch();
bool GPIOState();
extern gpioPin         gpio[]; 

#endif //GPIO_H
