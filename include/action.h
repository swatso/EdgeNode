#ifndef ACTION_H
#define ACTION_H

/*
  action.h - Library for controlling actions in the ESP32Node project
*/

//============ Includes ====================
#include <Arduino.h>
#include <FS.h>

void setupAction();
void loadActionConfig();                                           

#define CMD_ANY 0
#define CMD_LOCAL 1
#define CMD_REMOTE 2

// Action States
#define ACTION_STOPPED    0
#define ACTION_PLAYING    1
#define ACTION_PLAYED     2
#define ACTION_STOPPING   3

#define DEMAND_PLAY       10
//#define DEMAND_LOOP       11
#define DEMAND_STOP       12
#define DEMAND_FINNISH    13

//object to control an Action (animation sequence)
class edgeAction
{
  public:
    char name[20];            // Friendly name of the action
    uint8_t number;              // Action number (0-15)
    bool enableLocal;
    bool enableRemote;
    edgeAction();
    void play(uint8_t source, boolean repeat);
    void stop(uint8_t source);
    void finnish(uint8_t source);
    uint8_t getState();
    bool getRepeat() {return repeat;};
    void loadConfig(fs::FS &fs, uint8_t number);
    void saveConfig(fs::FS &fs, uint8_t number);
    void setActionPlayFunction(int (*fp)(uint8_t number));
    void setActionStopFunction(int (*fp)(uint8_t number));
    int userState; // this variable can be used by the user defined play and stop functions to keep track of the state
    int userVar1;  
    int userVar2; // this variable can be used by the user defined play and stop functions to keep track of a variable (e.g. current track number)
    void pause(long mS);
    void helper();                  // This is essentially private to the class but is called by the external helper task
                                    // FreeRtos does not allow this function to be instantiated as a task directly
  private:
    uint8_t state;
    uint8_t demandedState;
    bool repeat;
    int (*fpPlay)(uint8_t number) = nullptr;
    int (*fpStop)(uint8_t number) = nullptr;

};

void aHelper0(void * pvParameters); 
void aHelper1(void * pvParameters);
void aHelper2(void * pvParameters);
void aHelper3(void * pvParameters);
void aHelper4(void * pvParameters);
void aHelper5(void * pvParameters);
void aHelper6(void * pvParameters);
void aHelper7(void * pvParameters);
void aHelper8(void * pvParameters);
void aHelper9(void * pvParameters);
void aHelper10(void * pvParameters);
void aHelper11(void * pvParameters);
void aHelper12(void * pvParameters);
void aHelper13(void * pvParameters);
void aHelper14(void * pvParameters);
void aHelper15(void * pvParameters);

int defaultPlayFcn(uint8_t number);
int defaultStopFcn(uint8_t number);

extern edgeAction action[];

#endif //ACTION_H
