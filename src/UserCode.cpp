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
#include "sound.h"
#include "GCodeControl.h"

void setupUserCode() 
{
    // User code setup
    // This function is called during setup() in main.cpp and can be used to perform any user defined setup required for the action sequences, such as configuring GPIOs, initializing sensors, etc.
    // For example, to set the play and stop functions for Action 0 to the template functions defined below:

    // hardcode GPIO types here
    initGCodeControl();

    // Create a helper task for Marlin handshake processing
    /*if (xTaskCreatePinnedToCore(MarlinCNCHelper, "MarlinCNCTask", 3000, nullptr, 1, &MarlinCNCTask, 0) != pdPASS)
    {
      Serial.println("Failed to create MarlinCNCTask");
    }*/

    gpio[0].setType(GPIO_PWM_PULSE); // Set GPIO 0 to PWM mode (for example purposes, you can change this to any other type and GPIO as needed)
    gpio[0].preset0 = 250;    // Mark Period in mS (applies to GPIO_PWM_PULSE)
    gpio[0].preset1 = 20;    // Off PWM setting (0 to 255) (applies to GPIO_PWM_PULSE)
    gpio[0].preset2 = 200;   // On PWM setting (0 to 255) (applies to GPIO_PWM_PULSE)
    gpio[0].rate = 1000;     // Overall pulse cycle in mS (applies to GPIO_PWM_PULSE)
    
    gpio[0x0D].setType(GPIO_NONE);    // Used for serial interface to Marlin
    gpio[0x0E].setType(GPIO_NONE);    // Used for serial interface to Marlin

    // hardcode sound configuration here
    strcpy(mp3.track[0].name, "Ambient");
    mp3.track[0].duration = 50000;        // duration of track 0 in mS
    mp3.track[0].volume = 10;            // volume for track 0 (0 to 30)

    strcpy(mp3.track[1].name, "Ambient");
    mp3.track[1].duration = 78000;    // duration of track 1 in mS - Ambient
    mp3.track[1].volume = 5;        // volume for track 1 (0 to 30)

    strcpy(mp3.track[2].name, "Ambient2");
    mp3.track[2].duration = 77000;    // duration of track 2 in mS - Ambient
    mp3.track[2].volume = 3;        // volume for track 2 (0 to 30)

    strcpy(mp3.track[3].name, "Children Playing");
    mp3.track[3].duration = 72000;    // duration of track 3 in mS - Children playing
    mp3.track[3].volume = 10;        // volume for track 3 (0 to 30)

    strcpy(mp3.track[4].name, "Siren");
    mp3.track[4].duration = 4000;    // duration of track 4 in mS - Siren
    mp3.track[4].volume = 10;        // volume for track 4 (0 to 30)

    strcpy(mp3.track[5].name, "Train Bell");
    mp3.track[5].duration = 9000;    // duration of track 5 in mS - Train Bell
    mp3.track[5].volume = 10;        // volume for track 5 (0 to 30)

    strcpy(mp3.track[6].name, "Siren Low");
    mp3.track[6].duration = 19000;    // duration of track 6 in mS - Siren Low
    mp3.track[6].volume = 10;        // volume for track 6 (0 to 30)
    strcpy(mp3.track[7].name, "Whistle");
    mp3.track[7].duration = 1000;    // duration of track 7 in mS - Whistle
    mp3.track[7].volume = 10;        // volume for track 7 (0 to 30)

    strcpy(mp3.track[8].name, "Whistle Long");
    mp3.track[8].duration = 3000;    // duration of track 8 in mS  - Whistle
    mp3.track[8].volume = 10;        // volume for track 8 (0 to 30)
    
    strcpy(mp3.track[9].name, "Horn Short");
    mp3.track[9].duration = 1000;    // duration of track 9 in mS - Horn
    mp3.track[9].volume = 10;        // volume for track 9 (0 to 30)
    // ...  




    // define an example action with the template play and stop functions defined below. You can define up to 16 actions (numbered 0-15) with their own play and stop functions.
    strcpy(action[0].name, "Template Action");
    action[0].setActionPlayFunction(templatePlayFcn);
    action[0].setActionStopFunction(templateStopFcn);

    strcpy(action[1].name, "GCode Home Command");
    action[1].setActionPlayFunction(action1PlayFcn);
    action[1].setActionStopFunction(action1StopFcn);

    strcpy(action[2].name, "path14.0");
    action[2].setActionPlayFunction(action2PlayFcn);
    action[2].setActionStopFunction(action2StopFcn);

    strcpy(action[3].name, "path14.1");
    action[3].setActionPlayFunction(action3PlayFcn);
    action[3].setActionStopFunction(action3StopFcn);

    strcpy(action[4].name, "path15.0");
    action[4].setActionPlayFunction(action4PlayFcn);
    action[4].setActionStopFunction(action4StopFcn);

    strcpy(action[5].name, "path15.1");
    action[5].setActionPlayFunction(action5PlayFcn);
    action[5].setActionStopFunction(action5StopFcn);

    strcpy(action[6].name, "GCode Load Object");
    action[6].setActionPlayFunction(action6PlayFcn);
    action[6].setActionStopFunction(action6StopFcn);

    strcpy(action[7].name, "play Scene1");
    action[7].setActionPlayFunction(action7PlayFcn);
    action[7].setActionStopFunction(action7StopFcn);

    strcpy(action[8].name, "play SD file");
    action[8].setActionPlayFunction(action8PlayFcn);
    action[8].setActionStopFunction(action8StopFcn);

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
      mp3.play(CMD_LOCAL,5,false); // play sound track 5
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


int action1PlayFcn(uint8_t number)
{
  (void)number;
  const char* gcodeFilePath = "/home.gcode";

  if (!sendGCodeFile(gcodeFilePath))
  {
    localDebug.println("Action 1 failed to send G-code file: " + String(gcodeFilePath));
    return(0);
  }

  localDebug.println("Action 1 sent G-code file: " + String(gcodeFilePath));
  return(0);
}

int action1StopFcn(uint8_t number)
{
  action[number].stop(CMD_LOCAL);
  return(0);
}

int action2PlayFcn(uint8_t number)
{
  (void)number;
  const char* gcodeFilePath = "/Path14.0.gcode";

  if (!sendGCodeFile(gcodeFilePath))
  {
    localDebug.println("Action 2 failed to send G-code file: " + String(gcodeFilePath));
    return(0);
  }

  localDebug.println("Action 2 sent G-code file: " + String(gcodeFilePath));
  return(0);
}

int action2StopFcn(uint8_t number)
{
  action[number].stop(CMD_LOCAL);
  return(0);
}

int action3PlayFcn(uint8_t number)
{
  (void)number;
  const char* gcodeFilePath = "/Path14.1.gcode";

  if (!sendGCodeFile(gcodeFilePath))
  {
    localDebug.println("Action 3 failed to send G-code file: " + String(gcodeFilePath));
    return(0);
  }

  localDebug.println("Action 3 sent G-code file: " + String(gcodeFilePath));
  return(0);
}

int action3StopFcn(uint8_t number)
{
  action[number].stop(CMD_LOCAL);
  return(0);
}

int action4PlayFcn(uint8_t number)
{
  (void)number;
  const char* gcodeFilePath = "/Path15.0.gcode";

  if (!sendGCodeFile(gcodeFilePath))
  {
    localDebug.println("Action 4 failed to send G-code file: " + String(gcodeFilePath));
    return(0);
  }

  localDebug.println("Action 4 sent G-code file: " + String(gcodeFilePath));
  return(0);
}

int action4StopFcn(uint8_t number)
{
  action[number].stop(CMD_LOCAL);
  return(0);
}

int action5PlayFcn(uint8_t number)
{
  (void)number;
  const char* gcodeFilePath = "/Path15.1.gcode";

  if (!sendGCodeFile(gcodeFilePath)) // send in reverse
  {
    localDebug.println("Action 5 failed to send G-code file: " + String(gcodeFilePath));
    return(0);
  }

  localDebug.println("Action 5 sent G-code file: " + String(gcodeFilePath));
  return(0);
}

int action5StopFcn(uint8_t number)
{
  action[number].stop(CMD_LOCAL);
  return(0);
}



int action6PlayFcn(uint8_t number)
{
  loadGCodeObject();
  localDebug.println("Action 6 load GCode Object");
  return(0);
}

int action6StopFcn(uint8_t number)
{
  action[number].stop(CMD_LOCAL);
  return(0);
}

int action7PlayFcn(uint8_t number)
{
  (void)number;
  const char* filePath = "/scene1.txt";

  if (! sendGCodeFileList(filePath))
  {
    localDebug.println("Action 7 failed to send list: " + String(filePath));
    return(0);
  }
  
  localDebug.println("Action 7 sent list: " + String(filePath));
  return(0);
}

int action7StopFcn(uint8_t number)
{
  action[number].stop(CMD_LOCAL);
  return(0);
}

int action8PlayFcn(uint8_t number)
{
  (void)number;
  const char* gcodeFilePath = "/SDPath.gcode";

  if (!sendGCodeFile(gcodeFilePath))
  {
    localDebug.println("Action 8 failed to send G-code file: " + String(gcodeFilePath));
    return(0);
  }

  localDebug.println("Action 8 sent G-code file: " + String(gcodeFilePath));
  return(0);
}

int action8StopFcn(uint8_t number)
{
  action[number].stop(CMD_LOCAL);
  return(0);
}
