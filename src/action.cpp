
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
  xTaskCreatePinnedToCore(aHelper0, "ACTION0", 2000, NULL, 1, &actionTask0, 0); // Core 0
  xTaskCreatePinnedToCore(aHelper1, "ACTION1", 2000, NULL, 1, &actionTask1, 0); // Core 0
  xTaskCreatePinnedToCore(aHelper2, "ACTION2", 2000, NULL, 1, &actionTask2, 0); // Core 0
  xTaskCreatePinnedToCore(aHelper3, "ACTION3", 2000, NULL, 1, &actionTask3, 0); // Core 0
  xTaskCreatePinnedToCore(aHelper4, "ACTION4", 2000, NULL, 1, &actionTask4, 0); // Core 0
  xTaskCreatePinnedToCore(aHelper5, "ACTION5", 2000, NULL, 1, &actionTask5, 0); // Core 0
  xTaskCreatePinnedToCore(aHelper6, "ACTION6", 2000, NULL, 1, &actionTask6, 0); // Core 0
  xTaskCreatePinnedToCore(aHelper7, "ACTION7", 2000, NULL, 1, &actionTask7 ,0); // Core 0
  xTaskCreatePinnedToCore(aHelper8, "ACTION8", 2000, NULL, 1, &actionTask8, 0); // Core 0
  xTaskCreatePinnedToCore(aHelper9, "ACTION9", 2000, NULL, 1, &actionTask9, 0); // Core 0
  xTaskCreatePinnedToCore(aHelper10, "ACTION10", 2000, NULL, 1, &actionTask10, 0); // Core 0
  xTaskCreatePinnedToCore(aHelper11, "ACTION11", 2000, NULL, 1, &actionTask11, 0); // Core 0
  xTaskCreatePinnedToCore(aHelper12, "ACTION12", 2000, NULL, 1, &actionTask12, 0); // Core 0
  xTaskCreatePinnedToCore(aHelper13, "ACTION13", 2000, NULL, 1, &actionTask13 ,0);	// Core 0 
  xTaskCreatePinnedToCore(aHelper14, "ACTION14", 2000, NULL, 1, &actionTask14 ,0); // Core 0
  xTaskCreatePinnedToCore(aHelper15, "ACTION15", 2000, NULL, 1, &actionTask15 ,0);	// Core 0


}

void aHelper0(void * pvParameters)
{
  // helper task for Action 0 sequence control
  action[0].helper();
}

void aHelper1(void * pvParameters)
{
  // helper task for Action 1 sequence control
  action[1].helper();
}

void aHelper2(void * pvParameters)
{
  // helper task for Action 2 sequence control
  action[2].helper();
}

void aHelper3(void * pvParameters)
{
  // helper task for Action 3 sequence control
  action[3].helper();
}

void aHelper4(void * pvParameters)
{
  // helper task for Action 4 sequence control
  action[4].helper();
}

void aHelper5(void * pvParameters)
{
  // helper task for Action 5 sequence control
  action[5].helper();
}

void aHelper6(void * pvParameters)
{
  // helper task for Action 6 sequence control
  action[6].helper();
}

void aHelper7(void * pvParameters)
{
  // helper task for Action 7 sequence control
  action[7].helper();
}

void aHelper8(void * pvParameters)
{
  // helper task for Action 8 sequence control
  action[8].helper();
}

void aHelper9(void * pvParameters)
{
  // helper task for Action 9 sequence control
  action[9].helper();
}

void aHelper10(void * pvParameters)
{
  // helper task for Action 10 sequence control
  action[10].helper();
}

void aHelper11(void * pvParameters)
{
  // helper task for Action 11 sequence control
  action[11].helper();
}

void aHelper12(void * pvParameters)
{
  // helper task for Action 12 sequence control
  action[12].helper();
}

void aHelper13(void * pvParameters)
{
  // helper task for Action 13 sequence control
  action[13].helper();
}

void aHelper14(void * pvParameters)
{
  // helper task for Action 14 sequence control
  action[14].helper();
}

void aHelper15(void * pvParameters)
{
  // helper task for Action 15 sequence control
  action[15].helper();
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

void edgeAction::pause(long mS)
{
  // This function creates a delay in the execution of the action sequence. 
  // It is used to create timing between events in the action sequence. 
  //The delay is implemented using vTaskDelay, which allows other tasks to run while the action is paused.

  vTaskDelay(mS / portTICK_PERIOD_MS);
  return;
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
  action[number].pause(1000); // example of using the pause function to create a delay in the action sequence
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
        else if(repeat == true)state = ACTION_PLAYING;
        vTaskDelay(100);
        break;
    }
    yield(); // let other stuff run (we are on CPU1)
  }
}

edgeAction action[16];  // Array of edgeAction objects

