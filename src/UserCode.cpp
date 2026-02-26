// Example User Code Implementation
// define GPIO configuration and Action functions here
// You can define up to 16 actions (numbered 0-15) and each action can have a user defined play and stop function. 
// The play function will be called by the helper task to execute the action sequence based on the current state of the action. 
// The stop function will be called when the action is stopped to perform any stopping sequence required for the action. 

// For operational and debugging purposes, you can print to the following streams
// localDebug: this stream is intended for debugging purposes and publishes to the MQTTtopic "iot/debug/nodeID" (e.g. iot/debug/01). 
// localOperations: this stream is intended for operational messages and publishes to the MQTTtopic "iot/operations/nodeID" (e.g. iot/operations/01). 
// globalDebug: this stream is intended for debugging purposes and publishes to the MQTTtopic "iot/debug/global". 
// globalOperations: this stream is intended for operational messages and publishes to the MQTTtopic "iot/operations/global".

// It is important that the user defined Play and Stop functions do not block the helper task.
// So do not use 'delay()' in code to create a delay in the action sequence.
// Instead, implement the sequence as a state machine as shown in the template functions below.
// Use the provided 'state' variable to keep track of the current state of the sequence
// and each time that the sequnce needs tp pause, simply call 'return' with the required delay time in milliseconds.
// In this way the helper task can check if the action has be requested to stop and if so, will schedule the
// Stop functioninstead (otherwise it will call your Play function again after the delay time has elapsed to execute the next step in the sequence).

#include "UserCode.h"
#include "debugStream.h"
#include "gpio.h"
#include "action.h"

void setupUserCode() 
{
    // User code setup
    // This function is called during setup() in main.cpp and can be used to perform any user defined setup required for the action sequences, such as configuring GPIOs, initializing sensors, etc.
    // For example, to set the play and stop functions for Action 0 to the template functions defined below:

    // hardcode GPIO types here
    gpio[0].setType(GPIO_PWM_PULSE); // Set GPIO 0 to PWM mode (for example purposes, you can change this to any other type and GPIO as needed)
    gpio[0].preset0 = 250;    // Mark Period in mS (applies to GPIO_PWM_PULSE)
    gpio[0].preset1 = 20;    // Off PWM setting (0 to 255) (applies to GPIO_PWM_PULSE)
    gpio[0].preset2 = 200;   // On PWM setting (0 to 255) (applies to GPIO_PWM_PULSE)
    gpio[0].rate = 1000;     // Overall pulse cycle in mS (applies to GPIO_PWM_PULSE)

    // more GPIO configuration can be added here as needed for the action sequences
    // ...


    // define an example action with the template play and stop functions defined below. You can define up to 16 actions (numbered 0-15) with their own play and stop functions.
    strcpy(action[0].name, "Template Action");
    action[0].setActionPlayFunction(templatePlayFcn);
    action[0].setActionStopFunction(templateStopFcn);

    // define an example action to monitor RUN1 switch with the runSwitchHandler function defined below. This action will be scheduled to run every 500 mS to check the state of the RUN1 switch and start/stop the action sequence for action 0 based on the state of the switch.
    strcpy(action[15].name, "RUN1 switch Action");
    action[15].setActionPlayFunction(runSwitchHandler);
    action[15].play(CMD_LOCAL, true); // start the action to monitor the RUN1 switch (this will call the runSwitchHandler function every 500 mS to check the state of the switch and start/stop the action sequence for action 0 accordingly)
    localDebug.println("User code setup completed");
}

int runSwitchHandler(uint8_t number)
{
  // This function will be scheduled as action15 to check the state of the run1 switch
  switch(action[number].userState)
  {
    case 1:
      if(run1Switch())
      {
        localDebug.println("Run1 switch is ON, starting action sequence for action number: 0");
        action[0].play(CMD_LOCAL, true); // start the action sequence for action 0 (this will call the play function for action 0 to execute the first step in the sequence)
        action[number].userState = 2;
      }
      break;
    case 2:
      if(run1Switch()==false)
      {
        localDebug.println("Run1 switch is OFF, stopping action sequence for action number: 0");
        action[0].stop(CMD_LOCAL); 
        action[number].userState = 1;
      }
      break;
    }
    return(500); // check the switch states every 500 mS
}


// Provide Play & Stop functions for each action (animation sequence) here. 
// ========================================================================
// These functions will be called by the helper task for each action to execute the action sequence based on the current state of the action. The actual implementation of the action sequence will depend on the specific requirements of each action and may involve controlling GPIOs, playing sounds, etc. The userState, userVar1 and userVar2 variables can be used to keep track of the state of the action sequence and any other variables that may be needed for the execution of the action sequence. The current value of userState, userVar1 and userVar2 is displayed on the action configuration page in the web interface for debugging purposes.
// The following template functions provide an example

int templatePlayFcn(uint8_t number)
{
  //Serial.print("templatePlayFunction for action:"); Serial.println(number);

  // Note that the user defined play function may use the userState, userVar1 and userVar2 
  // variables to keep track of the state of the action sequence and any other variables 
  // that may be needed for the execution of the action sequence.
  // The current value of userState, userVar1 and userVar2 is displayed on the 
  //action configuration page in the web interface for debugging purposes.

  switch(action[number].userState)
  {
    case 1:
      // Perform the first step of the action sequence
      // For example, turn on a GPIO, play a sound, etc.
      localOperations.println("Executing action sequence for action number: " + String(number));
      localDebug.println("Step 1 of action sequence for action number: " + String(number));
      // Set userState to the next state and return the required delay time in milliseconds before the next step is executed
      gpio[0].rate = 1000;     // change the pulse cycle to 1000 mS (applies to GPIO_PWM_PULSE)
      gpio[0].localWrite(true); 
      action[number].userState = 2;
      return(10000); // wait for 10 seconds before executing the next step

    case 2:
      // Perform the second step of the action sequence
      localDebug.println("Step 2 of action sequence for action number: " + String(number));
      // Set userState to the next state and return the required delay time in milliseconds before the next step is executed
      gpio[0].rate = 500;     // change the pulse cycle to 500 mS (applies to GPIO_PWM_PULSE)
      action[number].userState = 3;
      return(20000); // wait for 20 seconds before executing the next step

    case 3:
      // Perform the third step of the action sequence
      localOperations.println("Step 3 of action sequence for action number: " + String(number));
      // Set userState to 0 to indicate that the sequence is completed and return 0 to indicate that no further steps are required
      gpio[0].localWrite(false);
      return(0); // no further steps required

    default:
      // Invalid state, reset userState and return 0 to stop the sequence
      localDebug.println("Invalid state in action sequence for action number: " + String(number) + ", aborting sequence");
      return(0); // stop the sequence
  } 
}

int templateStopFcn(uint8_t number)
{
  //Serial.print("templateStopFunction:"); Serial.println(number);

  // Perform any stopping sequence required for this action
  // Note that the user defined stop function can use the userState, userVar1 and userVar2 
  // variables to keep track of the state of the action sequence and any other variables 
  // that may be needed for the execution of the action sequence.
  // The current value of userState, userVar1 and userVar2 is displayed on the 
  //action configuration page in the web interface for debugging purposes.
  localOperations.println("Stopping action sequence for action number: " + String(number));
  gpio[0].localWrite(false);
  action[number].stop(CMD_LOCAL);
  return(0);
}
