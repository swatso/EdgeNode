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
#include "CraneWinch.h"

#define CRANE_MAX_DELAY 2000  // Maximum delay for crane rotation in milliseconds
#define CRANE_SLOW 128
#define CRANE_STOP 0
#define CRANE_CLOCKWISE 0
#define CRANE_ANTICLOCKWISE 1
#define CRANE_ROTATION_TIME 60000  // Time for crane rotation in milliseconds (60 seconds)
#define CEMENT_LORRY_BARREL_STOP 0
#define CEMENT_LORRY_BARREL_SLOW 200
#define CEMENT_LORRY_BARREL_FAST 255
#define LIGHTS_BRIGHT 64           // Cement Lorry four way flashers
#define LIGHTS_DIM 0
CraneStatus status;

// Setup the I2C interface for the CraneWinch module
TwoWire& wire = Wire;
uint8_t craneWinchAddress = 0x40; // replace with the actual I2C address of the CraneWinch module
uint8_t commandRegister = 0x01; // replace with the actual command register address
uint8_t statusRegister = 0x02; // replace with the actual status register address
CraneWinch craneWinch(wire, craneWinchAddress, commandRegister, statusRegister);

void setupUserCode() 
{
  if (!wire.begin(23,22))
  {
    Serial.println("Failed to initialize I2C for CraneWinch");
  }
  else
  {
    Serial.println("I2C for CraneWinch initialized successfully");
  }

  // Override the default target pulse counts for the crane winch
  craneWinch.writeTargetPulseCount(1, 10);
  craneWinch.writeTargetPulseCount(2, 42);
  craneWinch.writeTargetPulseCount(3, 61);

  // User code setup
    // This function is called during setup() in main.cpp and can be used to perform any user defined setup required for the action sequences, such as configuring GPIOs, initializing sensors, etc.
    // For example, to set the play and stop functions for Action 0 to the template functions defined below:

    // hardcode GPIO types here
    //initGCodeControl();     // GCode Control is not part of the Victoria Centre controller

    gpio[0].setType(GPIO_PWM);          // Crane rotation speed demand
    strcpy(gpio[0].name, "Crane rotation spd");
    gpio[0].preset1 = CRANE_STOP;
    gpio[0].preset2 = CRANE_SLOW;

    gpio[1].setType(GPIO_DIGOUT);       // Crane rotation direction
    strcpy(gpio[1].name, "Crane rotation dir");

    gpio[4].setType(GPIO_PWM);       // Cement Lorry Barrel
    strcpy(gpio[4].name, "Cement Lorry Barrel");
    gpio[4].localWrite(false);
    gpio[4].preset1 = CEMENT_LORRY_BARREL_STOP;
    gpio[4].preset2 = CEMENT_LORRY_BARREL_SLOW;

    gpio[5].setType(GPIO_PWM_PULSE);          // Crane lights
    strcpy(gpio[5].name, "4 way flashers");
    gpio[5].preset1 = LIGHTS_BRIGHT;
    gpio[5].preset2 = LIGHTS_DIM;
    gpio[5].rate = 2000;                  // cycle period in mS for the PWM pulse
    gpio[5].preset0 = 1000;                // Bright (strobe) period in mS

    gpio[6].setType(GPIO_PWM);          // Building Feature Lights
    strcpy(gpio[6].name, "Building Lights");
    gpio[6].preset1 = LIGHTS_DIM;
    gpio[6].localWrite(true);             // ALways flashing

    // hardcode sound configuration here
    strcpy(mp3.track[0].name, "Ambient");
    mp3.track[0].duration = 330000;        // duration of track 0 in mS
    mp3.track[0].volume = 30;            // volume for track 0 (0 to 30)

    strcpy(mp3.track[1].name, "CementLorry");
    mp3.track[1].duration = 82000;    // duration of track 1 in mS
    mp3.track[1].volume = 20;        // volume for track 1 (0 to 30)

    strcpy(mp3.track[2].name, "none");
    mp3.track[2].duration = 0;    // duration of track 2 in mS
    mp3.track[2].volume = 3;        // volume for track 2 (0 to 30)

    strcpy(mp3.track[3].name, "none");
    mp3.track[3].duration = 0;    // duration of track 3 in mS
    mp3.track[3].volume = 10;        // volume for track 3 (0 to 30)

    // define an example action with the template play and stop functions defined below. You can define up to 16 actions (numbered 0-15) with their own play and stop functions.
    strcpy(action[0].name, "Crane Automation");
    action[0].setActionPlayFunction(action0PlayFcn);
    action[0].setActionStopFunction(action0StopFcn);

    strcpy(action[1].name, "Crane Clockwise");
    action[1].setActionPlayFunction(action1PlayFcn);
    action[1].setActionStopFunction(action1StopFcn);

    strcpy(action[2].name, "Crane AntiClockwise");
    action[2].setActionPlayFunction(action2PlayFcn);
    action[2].setActionStopFunction(action2StopFcn);

    strcpy(action[3].name, "Winch P1");
    action[3].setActionPlayFunction(action3PlayFcn);
    action[3].setActionStopFunction(action3StopFcn);

    strcpy(action[4].name, "Winch P2");
    action[4].setActionPlayFunction(action4PlayFcn);
    action[4].setActionStopFunction(action4StopFcn);

    strcpy(action[5].name, "Winch P3");
    action[5].setActionPlayFunction(action5PlayFcn);
    action[5].setActionStopFunction(action5StopFcn);

    strcpy(action[6].name, "Cement Lorry On");
    action[6].setActionPlayFunction(action6PlayFcn);
    action[6].setActionStopFunction(action6StopFcn);

    strcpy(action[7].name, "Cement Lorry Off");
    action[7].setActionPlayFunction(action7PlayFcn);
    action[7].setActionStopFunction(action7StopFcn);

    strcpy(action[8].name, "Lorry Flashers");
    action[8].setActionPlayFunction(action8PlayFcn);
    action[8].setActionStopFunction(action8StopFcn);

    // define an example action to monitor RUN1 switch with the runSwitchHandler function defined below. This action will be scheduled to run every 500 mS to check the state of the RUN1 switch and start/stop the action sequence for action 0 based on the state of the switch.
    strcpy(action[15].name, "RUN1 switch Action");
    action[15].setActionPlayFunction(runSwitchHandler);
    action[15].play(CMD_LOCAL, true); // start the action to monitor the RUN1 switch (this will call the runSwitchHandler function every 500 mS to check the state of the switch and start/stop the action sequence for action 0 accordingly)

    localDebug.println("User code setup completed");
}


void initUserCode()
{
  // initialise outputs just prior to turning on power to the output subsystem
  // this is important if you want to over-ride the saved value of a servo at powerup for example

    mp3.play(CMD_LOCAL,0, true);          // play ambient sounds
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
      //gpio[0].rate = 1000;     // change the pulse cycle to 1000 mS (applies to GPIO_PWM_PULSE)
      //gpio[0].localWrite(true); 
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
  return(0);
}

int action0PlayFcn(uint8_t number)
{
  // Note: Crane rotation Limit switches are active LOW
  //  This is the main crane automated control action
  // It busies the crane moving a concrete load between two locations
  // With random delays between actions to simulate realistic operation
  switch(action[number].userState)
  {
    case 1:
    // Rotation Position unknown and raise winch to Position 1
    gpio[0].localWrite(CRANE_STOP);              // Make sure that rotation is stopped
    craneWinch.sendCommand(CraneCommand::MoveToPosition1);
    if(craneWinch.readStatus(status) && status == CraneStatus::Position1)
    {
      // Ok, the load is raised to a safe height to rotate the crane
      gpio[1].localWrite(CRANE_ANTICLOCKWISE);        // Crane rotation direction
      gpio[0].localWrite(CRANE_SLOW);                 // Crane rotation speed demand
      action[number].userState = 2;
      return(CRANE_ROTATION_TIME);
    }
    return(5000);         // delay whilst the winch operates
    break;

    case 2:
    // Rotated AntiClockwise and Lowering Hoist down    
    gpio[0].localWrite(CRANE_STOP);               // Make sure that rotation is stopped
    craneWinch.sendCommand(CraneCommand::MoveToPosition2);
    if(craneWinch.readStatus(status) && status == CraneStatus::Position2)
    {
      action[number].userState = 4;               // limit switch 0 is activated
    }
    return(100);
    break;

    case 3:
    return(1000);
    break;

    case 4:
    // Rotated Anticlockwise and Hoist is down
    gpio[0].localWrite(CRANE_STOP);               // Make sure that rotation is stopped
    action[number].userState = 6;
    return(random(1000, CRANE_MAX_DELAY));
    break;

    case 5:
    break;

    case 6:
    // Rotated Anticlockwise and Hoist travelling up
    gpio[0].localWrite(CRANE_STOP);               // Make sure that rotation is stopped
    craneWinch.sendCommand(CraneCommand::MoveToPosition1);      // transit position
    if(craneWinch.readStatus(status) && status == CraneStatus::Position1)
    {
      action[number].userState = 7;
    }
    return(100);
    break;

    case 7:
    // Rotated Anticlockwise and Hoist has reached the transit position (1)
    gpio[0].localWrite(CRANE_STOP);               // Make sure that rotation is stopped
    action[number].userState = 8;
    return(random(1000, CRANE_MAX_DELAY));
    break;

    case 8:
    // rotate clockwise
    gpio[1].localWrite(CRANE_CLOCKWISE);        // Crane rotation direction
    gpio[0].localWrite(CRANE_SLOW);              // Crane rotation speed demand
    action[number].userState = 9;
    mp3.stop(CMD_LOCAL);
    mp3.play(CMD_LOCAL,1, true);          // play lorry sound
    gpio[5].localWrite(true);             // turn on 4 way flashers
    gpio[4].localWrite(true);             // Turn on Cement Lorry barrel at slow speed
    return(CRANE_ROTATION_TIME);
    break;

    case 9:
    // Rotated clockwise and Hoist is in transit position
    gpio[0].localWrite(CRANE_STOP);              // Crane rotation speed demand
    action[number].userState = 10;

    // turn on the cement lorry barrel and 4 way flashers

    gpio[4].localWrite(true);
    gpio[5].localWrite(true);
    return(1000);
    break;

    case 10:
    // Rotated clockwise and hoist moving downwards
    gpio[4].localWrite(true);
    gpio[0].localWrite(CRANE_STOP);               // Make sure that rotation is stopped
    craneWinch.sendCommand(CraneCommand::MoveToPosition3);
    if(craneWinch.readStatus(status) && status == CraneStatus::Position3)
    {
      action[number].userState = 12;            
    }
    return(100);
    break;

    case 11:
    break;

    case 12:
    // Rotated clockwise and Hoist is down
    gpio[0].localWrite(CRANE_STOP);              // Make sure that rotation is stopped
    action[number].userState = 14;
    return(random(1000, CRANE_MAX_DELAY));
    break;

    case 13:
    break;

    case 14:
    // Rotated clockwise and Hoist moving upwards to transit position (1)
    gpio[0].localWrite(CRANE_STOP);               // Make sure that rotation is stopped
    craneWinch.sendCommand(CraneCommand::MoveToPosition1);      // transit position
    if(craneWinch.readStatus(status) && status == CraneStatus::Position1)
    {
      action[number].userState = 15;
    }
    return(100);
    break;

    case 15:
    // Rotated clockwise and Hoist has reached the transit position
    gpio[4].localWrite(false);                    // stop the cement lorry barrel
    gpio[5].localWrite(false);                    // turn off 4 way flashers
    gpio[0].localWrite(CRANE_STOP);               // Make sure that rotation is stopped
    mp3.stop(CMD_LOCAL);
    mp3.play(CMD_LOCAL,0, true);                  // play ambient sound
    action[number].userState = 16;
    return(random(1000, CRANE_MAX_DELAY));
    break;

    case 16:
    // rotate anticlockwise
    gpio[1].localWrite(CRANE_ANTICLOCKWISE);        // Crane rotation direction
    gpio[0].localWrite(CRANE_SLOW);                 // Crane rotation speed demand
    action[number].userState = 2;
    return(random(CRANE_ROTATION_TIME, CRANE_MAX_DELAY));
    break;
  }
  return(0);
}

int action0StopFcn(uint8_t number)
{
  gpio[4].localWrite(false);                    // stop the cement lorry barrel
  gpio[5].localWrite(false);                    // turn off 4 way flashers
  gpio[0].localWrite(CRANE_STOP);               // Make sure that rotation is stopped
  mp3.stop(CMD_LOCAL);
  mp3.play(CMD_LOCAL,0 , true);      // play ambient sound
  return(0);
}


int action1PlayFcn(uint8_t number)
{
  Serial.println("action1PlayFcn: " + String(number));
  Serial.println("Rotate Crane Clockwise");
  switch(action[number].userState)
  {
    case 1:
    gpio[1].localWrite(CRANE_CLOCKWISE);        // Crane rotation direction
    gpio[0].localWrite(true); 
    action[number].userState = 2;
    return(50000);
    break;

    case 2:
    gpio[0].localWrite(false); 
    action[number].stop(CMD_LOCAL);             // Stop the action locally
    return(0);
    break;
  }
  return(0);
}

int action1StopFcn(uint8_t number)
{
  gpio[0].localWrite(false);
  return(0);
}

int action2PlayFcn(uint8_t number)
{
   Serial.println("action2PlayFcn: " + String(number));
  Serial.println("Rotate Crane Anticlockwise");
  switch(action[number].userState)
  {
    case 1:
    gpio[1].localWrite(CRANE_ANTICLOCKWISE);        // Crane rotation direction
    gpio[0].localWrite(true);              // Crane rotation speed demand
    action[number].userState = 2;
    return(50000);
    break;

    case 2:
    gpio[0].localWrite(false);          // Crane rotation speed demand
    action[number].stop(CMD_LOCAL);     // Go straight  to Stop State
    return(0);
    break;
  }
  return(0);
}

int action2StopFcn(uint8_t number)
{
  gpio[0].localWrite(false);          // Crane rotation speed demand
  return(0);
}

int action3PlayFcn(uint8_t number)
{  
  // Crane Winch Control for Moving to Position 1
  switch(action[number].userState)
  {
    case 1:
    craneWinch.sendCommand(CraneCommand::MoveToPosition1);
    action[number].userState = 2;
    return(100);
    break;

    case 2:
    Serial.println("Result of readStatus: " + String(craneWinch.readStatus(status)));

    if(craneWinch.readStatus(status) && status == CraneStatus::Position1) {
      Serial.println("Crane has reached Position 1");
      action[number].stop(CMD_LOCAL);     // Go straight  to Stop State
      return(0);
    }
    return(100);
  }
  return(0);
}

int action3StopFcn(uint8_t number)
{
  return(0);
}

int action4PlayFcn(uint8_t number)
{
  // Move Crane Winch to Position 2
  craneWinch.sendCommand(CraneCommand::MoveToPosition2);
  action[number].stop(CMD_LOCAL);     // Go straight  to Stop State
  return(0);
}

int action4StopFcn(uint8_t number)
{
  return(0);
}

int action5PlayFcn(uint8_t number)
{
  // Move Crane Winch to Position 3
  craneWinch.sendCommand(CraneCommand::MoveToPosition3);
  action[number].stop(CMD_LOCAL);     // Go straight  to Stop State
  return(0);
}

int action5StopFcn(uint8_t number)
{
  action[number].stop(CMD_LOCAL);
  return(0);
}



int action6PlayFcn(uint8_t number)
{
  Serial.println("action6PlayFcn: " + String(number));
  Serial.println("Cement Lorry On");
  gpio[4].localWrite(true);  // Turn on Cement Lorry barrel at slow speed
  gpio[5].localWrite(true);  // Turn on 4 way flashers
  mp3.stop(CMD_LOCAL);
  mp3.play(CMD_LOCAL,1, true);      // play lorry sound
  action[number].stop(CMD_LOCAL);     // Go straight  to Stop State
  return(0);
}

int action6StopFcn(uint8_t number)
{
  action[number].stop(CMD_LOCAL);
  gpio[5].localWrite(false);  // Turn off 4 way flashers
  mp3.stop(CMD_LOCAL);
  mp3.play(CMD_LOCAL,0 , true);      // play ambient sound
  return(0);
}

int action7PlayFcn(uint8_t number)
{
  Serial.println("action7PlayFcn: " + String(number));
  Serial.println("Cement Lorry Off");
  gpio[4].localWrite(false);  // Turn off Cement Lorry barrel
  mp3.stop(CMD_LOCAL);
  mp3.play(CMD_LOCAL,0, true);  // Resume ambient sounds
  action[number].stop(CMD_LOCAL);     // Go straight  to Stop State
  return(0);
}

int action7StopFcn(uint8_t number)
{
  action[number].stop(CMD_LOCAL);
  return(0);
}

int action8PlayFcn(uint8_t number)
{
  return(0);
}

int action8StopFcn(uint8_t number)
{
  action[number].stop(CMD_LOCAL);
  return(0);
}
