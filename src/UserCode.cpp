// Example User Code Implementation
//
// HOW TO ADD A SCENE RUN FUNCTION
// 1. Write a function with the signature: bool sceneNRunFcn(uint8_t number)
//    (it receives the scene index and returns true on success).
// 2. Declare it in UserCode.h.
// 3. Register it in setupUserCode(): scenes[N].setRunFunction(sceneNRunFcn);
// Scene::run() then calls it. Scenes with no registered function fall back to running their SD path.
// Note: scene indexes are assigned in initGCodeControl(), so it must be called for 'number' to be correct.
//
// HOW TO REGISTER SCENE COMPLETION CALLBACKS
// 1. Write a callback function with the signature: void sceneCompletionFcn(uint8_t sceneIndex, bool success)
//    (it receives the scene index and execution result).
// 2. Register it in setupUserCode(): sceneCallbacks.registerCallback(sceneIndex, sceneCompletionFcn);
// The callback will be invoked after the scene completes execution (whether successful or failed).
// You can register multiple callbacks per scene (up to 4) and they execute sequentially.
// This is useful for post-scene cleanup, logging, triggering actions, or updating UI state.
//
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

// I had problems in this branch with the number of tasks being created (due to the Marlin CNC interface requirements)
// So we have just one action task handling all necessary animations. It accepts requests
// via the MQTT interface and executes the appropriate GCode routines together with sound effects and 
// minimal GPIO attached control logic (eg reading pallet presence sensors, controlling locks)

#include "UserCode.h"
#include "debugStream.h"
#include "gpio.h"
#include "action.h"
#include "sound.h"
#include "GCodeControl.h"
#include "WiFiManager.h"
#include "SceneCallbacks.h"

void setupUserCode() 
{
    // User code setup
    // This function is called during setup() in main.cpp and can be used to perform any user defined setup required for the action sequences, such as configuring GPIOs, initializing sensors, etc.
    // For example, to set the play and stop functions for Action 0 to the template functions defined below:

    // hardcode GPIO types here
    initGCodeControl();     // GCode Control is not part of the Victoria Centre controller

    gpio[0].setType(GPIO_DIGIN);          // Lorry Pallet Present Indicator (active low)
    strcpy(gpio[0].name, "Lorry Pallet");
    gpio[1].setType(GPIO_DIGOUT);       // Lorry Pallet Lock
    strcpy(gpio[1].name, "Lorry Pallet Lock");
    gpio[2].setType(GPIO_DIGIN);        // Yard Pallet Present Indicator (active low)
    strcpy(gpio[2].name, "Yard Pallet");
    gpio[3].setType(GPIO_DIGOUT);        // Yard Pallet Lock
    strcpy(gpio[3].name, "Yard Pallet Lock");
    gpio[4].setType(GPIO_DIGOUT_PULSE);        // Puck Keepalive output
    strcpy(gpio[4].name, "Puck Keepalive");
    gpio[4].preset0 = 30000;          // Mark duration for the pulse in mS
    gpio[4].rate = 60000;             // Overall pulse cycle in mS

    // hardcode sound configuration here
    strcpy(mp3.track[0].name, "Ambient");
    mp3.track[0].duration = 330000;        // duration of track 0 in mS
    mp3.track[0].volume = 15;            // volume for track 0 (0 to 30)

    strcpy(mp3.track[1].name, "Ambient");
    mp3.track[1].duration = 82000;    // duration of track 1 in mS - Ambient
    mp3.track[1].volume = 9;        // volume for track 1 (0 to 30)

    strcpy(mp3.track[2].name, "Ambient2");
    mp3.track[2].duration = 240000;    // duration of track 2 in mS - Ambient
    mp3.track[2].volume = 15;        // volume for track 2 (0 to 30)

    strcpy(mp3.track[3].name, "Fork Lift");
    mp3.track[3].duration = 240000;    // duration of track 3 in mS - Children playing
    mp3.track[3].volume = 20;        // volume for track 3 (0 to 30)

    strcpy(mp3.track[4].name, "Siren");
    mp3.track[4].duration = 4000;    // duration of track 4 in mS - Siren
    mp3.track[4].volume = 10;        // volume for track 4 (0 to 30)

    strcpy(mp3.track[5].name, "Train Bell");
    mp3.track[5]
    .duration = 9000;    // duration of track 5 in mS - Train Bell
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

    // Link scenes to their user-defined run functions (scenes without one run their SD path)
    scenes[2].setRunFunction(scene2RunFcn);
    scenes[3].setRunFunction(scene3RunFcn);
    scenes[8].setRunFunction(scene8RunFcn);
    scenes[9].setRunFunction(scene9RunFcn);

    // Register callbacks for scene completion
    sceneCallbacks.registerCallback(2, scene2CompletionHandler);
    sceneCallbacks.registerCallback(3, scene3CompletionHandler);
    sceneCallbacks.registerCallback(4, scene4CompletionHandler);
    sceneCallbacks.registerCallback(5, scene5CompletionHandler);
    sceneCallbacks.registerCallback(6, scene6CompletionHandler);
    sceneCallbacks.registerCallback(7, scene7CompletionHandler);
    sceneCallbacks.registerCallback(9, scene9CompletionHandler);
    localDebug.println("User code setup completed");
}



// Scene run and callback functions, typically invoked by Scene::run() with the scene index
bool scene8RunFcn(uint8_t number) 
{
  // Scene8 is a special scene which controls the ForkLift operations
  // it checks where the load (Pallet) is and triggers the appropriate
  // move scene based on the Pallet's location

  Serial.println("Running scene 8");
  if(gpio[0].localRead()==false)  // Check if the Pallet is on the Lorry
  {
    queueSceneRun(2);  // Move from Home to Lorry
  }
  else
  {
    queueSceneRun(3);  // Move from Home to Store
  }
  return true;
}

bool scene2RunFcn(uint8_t number)
{
  // Moves the load (pallet) from the Lorry to the Yard Store
  // Start by moving the puck to the ForkLift Home
  Serial.println("Running scene 2");
  gpio[3].localWrite(false);  // prepare the yard lock to close when the pallet arrives
  gpio[1].localWrite(true);  // Unlock the Lorry Pallet

  return queueSceneRun(number);
}

void scene2CompletionHandler(uint8_t sceneIndex, bool success)
{
  if (success)
  {
    // drive the ForkLift from home to the Lorry
     mp3.play(CMD_LOCAL, 3, false);  // Play track 3 when scene movement starts
    queueSceneRun(5);  // Run scene 5 to move the pallet from the Lorry to the Yard Store
  }
  else
  {
    // Handle scene failure
    mp3.play(CMD_LOCAL, 4, false);  // Play error sound
  }
}

bool scene3RunFcn(uint8_t number)
{
  // Moves the load (pallet) from the Yard Store to the Lorry
  // Start by moving the puck to the ForkLift Home
  Serial.println("Running scene 3");
  return queueSceneRun(number);
}

void scene3CompletionHandler(uint8_t sceneIndex, bool success)
{
  if (success)
  {
    // Forklift has arrived at store - unlock the Pallet
    gpio[3].localWrite(true);  // Unlock the Yard Pallet
    gpio[1].localWrite(false);  // prepare the lorry lock to closewhen the pallet arrives
    mp3.play(CMD_LOCAL, 3, false);  // Play track 3 when scene movement starts
    queueSceneRun(4);  // Run scene 4 to move the pallet from the Yard Store to the Lorry
  }
  else
  {
    // Handle scene failure
    mp3.play(CMD_LOCAL, 4, false);  // Play error sound
  }
}

void scene4CompletionHandler(uint8_t sceneIndex, bool success)
{
  if (success)
  {
    // ForkLift has arrived at the Lorry
    queueSceneRun(6);  // Return to Home
  }
  else
  {
    // Handle scene failure
    mp3.play(CMD_LOCAL, 4, false);  // Play error sound
  }
}

void scene5CompletionHandler(uint8_t sceneIndex, bool success)
{
  if (success)
  {
    // Pallet has arrived at Yard Store
    queueSceneRun(7);  // return to Home after delivering the pallet to the Yard Store
  }
  else
  {
    // Handle scene failure
    mp3.play(CMD_LOCAL, 4, false);  // Play error sound
  }
}

void scene6CompletionHandler(uint8_t sceneIndex, bool success)
{
  if (success)
  {
    // Perform post-scene actions on success
//    mp3.play(CMD_LOCAL, 5, false);  // Play success sound for scene 6
  }
  else
  {
    // Handle scene failure
    mp3.play(CMD_LOCAL, 4, false);  // Play error sound
  }
}

void scene7CompletionHandler(uint8_t sceneIndex, bool success)
{
  if (success)
  {
    // Perform post-scene actions on success
    mp3.play(CMD_LOCAL, 5, false);  // Play success sound for scene 7
  }
  else
  {
    // Handle scene failure
    mp3.play(CMD_LOCAL, 4, false);  // Play error sound
  }
}

bool scene9RunFcn(uint8_t number)
{
  // Moves the load (pallet) from a parking area to the Lorry, pallet already on ForkLift
  Serial.println("Running scene 9");
  mp3.play(CMD_LOCAL, 3, false);  // Play track 3 when scene 9 runs
  gpio[1].localWrite(false);  // prepare the lorry lock to closewhen the pallet arrives
  return queueSceneRun(number);
}

void scene9CompletionHandler(uint8_t sceneIndex, bool success)
{
  if (success)
  {
    // Perform post-scene actions on success
    mp3.play(CMD_LOCAL, 5, false);  // Play success sound for scene 9
    gpio[1].localWrite(true);  // unlock the pallet lock on the lorry
    queueSceneRun(10);  // return the pallet back to the lorry
  }
  else
  {
    // Handle scene failure
    mp3.play(CMD_LOCAL, 4, false);  // Play error sound
  }
}