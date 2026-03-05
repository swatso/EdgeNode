
/* Node Edge Action Control Module

 * This module defines the edgeAction class, which encapsulates the data and functions for controlling an action (animation sequence) in the ESP32Node project. Each action has a friendly name, a state, a demanded state, and a repeat flag. The play() function starts the action, the stop() function stops it, and the finnish() function is called when the action is completed. The pause() function is used to create delays in the execution of the action.
 *  these are sequences of events triggered by MQTT messages or local button presses, which may include playing a sound, flashing an LED, etc. The edgeAction class allows for the control of these sequences, including the ability to repeat them if desired.
 *
 *            
 */

#include <SoftwareSerial.h>
#include <Ticker.h>
#include "action.h"
#include "sound.h"  
#include "FileSystem.h"
#include "gpio.h"

#define MAX_ACTIONS 16

// Action helper tasks for each action (animation sequence)
TaskHandle_t actionTask0;
TaskHandle_t actionTask1;
TaskHandle_t actionTask2;
TaskHandle_t actionTask3;
TaskHandle_t actionTask4;
TaskHandle_t actionTask5;
TaskHandle_t actionTask6;
TaskHandle_t actionTask7;
TaskHandle_t actionTask8;
TaskHandle_t actionTask9;
TaskHandle_t actionTask10;
TaskHandle_t actionTask11;
TaskHandle_t actionTask12;
TaskHandle_t actionTask13;
TaskHandle_t actionTask14;
TaskHandle_t actionTask15;
static uint8_t actionTaskIndex[MAX_ACTIONS];
TaskHandle_t* actionTaskHandleRefs[MAX_ACTIONS] = {
  &actionTask0, &actionTask1, &actionTask2, &actionTask3,
  &actionTask4, &actionTask5, &actionTask6, &actionTask7,
  &actionTask8, &actionTask9, &actionTask10, &actionTask11,
  &actionTask12, &actionTask13, &actionTask14, &actionTask15
};

void loadActionConfig()
{
  for(uint8_t i=0; i<MAX_ACTIONS; i++) action[i].loadConfig(SPIFFS, i); 
}

void setupAction()
{
  // call this function at the end of setup() to initialise the MP3 player and volume control
  Serial.println("(setupAction)");

  for(uint8_t i=0; i<MAX_ACTIONS; i++)action[i].number = i;

  // Create helper tasks for each action (animation sequence)
  for(uint8_t i=0; i<MAX_ACTIONS; i++)
  {
    actionTaskIndex[i] = i;
    char taskName[9];
    snprintf(taskName, sizeof(taskName), "ACTION%u", i);
    if (xTaskCreatePinnedToCore(actionHelperTask, taskName, 3000, &actionTaskIndex[i], 1, actionTaskHandleRefs[i], 0) != pdPASS)
    {
      Serial.printf("Failed to create ACTION%u\n", i);
    }
  }


}

void actionHelperTask(void * pvParameters)
{
  uint8_t actionNumber = *(static_cast<uint8_t*>(pvParameters));
  while(true)
  {
    if(GPIOState()) action[actionNumber].helper();
    else vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}


edgeAction::edgeAction()
{
  state = ACTION_STOPPED;
  demandedState = DEMAND_STOP;
  enableLocal = true;
  enableRemote = true;
  userState = 1;
  userVar1 = 0;
  userVar2 = 0;
  setActionPlayFunction(defaultPlayFcn);
  setActionStopFunction(defaultStopFcn);
}

void edgeAction::loadConfig(fs::FS &fs, uint8_t number)
{
  // call this function at the end of setup() to load the edgeAction configuration from SPIFFS
  readActionConfigFile(SPIFFS, number);
}

void edgeAction::saveConfig(fs::FS &fs, uint8_t number)
{
  // call this function at the end of setup() to save the edgeAction configuration to SPIFFS
  writeActionConfigFile(SPIFFS, number);
}

void edgeAction::play(uint8_t source, boolean Actrepeat)
{
  // Plays the action if the source of the play command is allowed to play the action. This allows for local and remote control of the action, with the option to restrict certain actions to local or remote control only.
  if((source==CMD_ANY)||((source==CMD_LOCAL)&&(enableLocal==true))||((source==CMD_REMOTE)&&(enableRemote==true)))
  {
Serial.print("(play) action:");
Serial.println(name);
    demandedState = DEMAND_PLAY;
    repeat = Actrepeat;
  }
}

void edgeAction::stop(uint8_t source)
{
  // Stops the currently playing action if the source of the stop command is allowed to stop the action. This allows for local and remote control of the action, with the option to restrict certain actions to local or remote control only.
  if((source==CMD_ANY)||((source==CMD_LOCAL)&&(enableLocal==true))||((source==CMD_REMOTE)&&(enableRemote==true)))
  {
    Serial.print("(stop) action:");
    Serial.println(name);
    demandedState = DEMAND_STOP;
    repeat = false;
  }
}

void edgeAction::finnish(uint8_t source)
{
  // Finishes the currently playing action if the source of the finish command is allowed to finish the action. This allows for local and remote control of the action, with the option to restrict certain actions to local or remote control only.
  if((source==CMD_ANY)||((source==CMD_LOCAL)&&(enableLocal==true))||((source==CMD_REMOTE)&&(enableRemote==true)))
  {
    Serial.print("(finish) action:");
    Serial.println(name);
    demandedState = DEMAND_FINNISH;
    repeat = false;
  }
}

uint8_t edgeAction::getState()
{
  // Returns the current state of the action (idle, playing, or finishing)
  return state;
}

void edgeAction::setActionPlayFunction(int (*fp)(uint8_t number))
{
  fpPlay = fp;
}

void edgeAction::setActionStopFunction(int (*fp)(uint8_t number))
{
  fpStop = fp;
}

int defaultPlayFcn(uint8_t number)
{
  //Serial.print("defaultPlayFunction for action:"); Serial.println(number);

  // Note that the user defined play function can use the userState, userVar1 and userVar2 
  // variables to keep track of the state of the action sequence and any other variables 
  // that may be needed for the execution of the action sequence.
  // The current value of userState, userVar1 and userVar2 is displayed on the 
  //action configuration page in the web interface for debugging purposes.
  action[number].userState = 1;
  action[number].userVar1 = 0;
  action[number].userVar2++;
  return(0);                       // return the delay time in milliseconds until the next state transition in the action sequence. 
                                      // If the action sequence has finished, return 0 to indicate that there are no more state transitions and the action can be set to idle.
}

int defaultStopFcn(uint8_t number)
{
  //Serial.print("defaultStopFunction:"); Serial.println(number);

  // Perform any stopping sequence required for this action
  // Note that the user defined stop function can use the userState, userVar1 and userVar2 
  // variables to keep track of the state of the action sequence and any other variables 
  // that may be needed for the execution of the action sequence.
  // The current value of userState, userVar1 and userVar2 is displayed on the 
  //action configuration page in the web interface for debugging purposes.
  return(0);
}

void edgeAction::helper()
{
  // This is the helper function for the action sequence control. 
  // It is called by the helper task for each action (animation sequence) and is 
  // responsible for executing the action sequence based on the current state of the action. 
  // The actual implementation of the action sequence will depend on the specific requirements 
  // of each action and may involve controlling GPIOs, playing sounds, etc.
  int delayTime;
  while(true)
  {
    switch(state)
    {
      case ACTION_STOPPED:
        if(demandedState == DEMAND_PLAY)state = ACTION_PLAYING;
        else
        {
          userState = 1;                  // reset userState for the next run action
          demandedState = DEMAND_STOP;
          vTaskDelay(100);
        }
        break;

      case ACTION_STOPPING:
        // execute any cleanup or finishing behavior for the action
        // this could involve stopping sounds, resetting GPIOs, etc.
        delayTime = fpStop(number);       // call the user defined stop function to execute the stopping behavior and get the delay time for the next state transition
        if(delayTime == 0)
        {
          repeat = false;
          state = ACTION_STOPPED; // reset to idle -completed finishing work
        }
        else  vTaskDelay(delayTime / portTICK_PERIOD_MS);
        break;

      case ACTION_PLAYING:
        // execute the action sequence
        delayTime = fpPlay(number);       // call the user defined play function to execute the action behavior and get the delay time for the next state transition
        if(delayTime == 0)
        {
          // User sequence has finished
          if(demandedState == DEMAND_FINNISH)repeat = false;
          state = ACTION_PLAYED;
          vTaskDelay(100);
          break;
        }
        // User sequence is still playing...
        if(demandedState == DEMAND_STOP)
        {
          userState = 1;              // reset userState for the stopping action
          state = ACTION_STOPPING;
          repeat = false;
        }
        else vTaskDelay(delayTime / portTICK_PERIOD_MS);
        break;

        case ACTION_PLAYED:
        // the action sequence has completed
        if((demandedState == DEMAND_FINNISH) || (demandedState == DEMAND_STOP))
        {
          userState = 1;              // reset userState for the stopping action
          state = ACTION_STOPPING;
          repeat = false;
        }
        else if(repeat == true)
        {
          state = ACTION_PLAYING;
          userState = 1;              // reset userState for the next run through the action sequence
        }
        vTaskDelay(100);
        break;
    }
    yield(); // let other stuff run (we are on CPU1)
  }
}

edgeAction action[16];  // Array of edgeAction objects

