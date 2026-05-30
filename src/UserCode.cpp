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
#include "marlin_handshake.h"
#include <SPIFFS.h>

HardwareSerial gcodeUart(1);  // UART1 (use 1 or 2 typically)
MarlinHandshake<> handshake(gcodeUart);
/*TaskHandle_t MarlinCNCTask;
char GCodeLine[128]; // Buffer to hold the next G-code line to send to Marlin

void MarlinCNCHelper(void * pvParameters)
{
  while(true)
  {
    handshake.processInput();
    if(handshake.canSendNow())
    {
      // Send the next command to Marlin if there are no commands in flight
      // For example, you could read the next command from a queue and send it using handshake.sendLine(command);
      if(strlen(GCodeLine) != 0)
      {
        handshake.sendLine(GCodeLine); // Send the next G-code line to Marlin
        Serial.println("Sent G-code to Marlin:");
        Serial.println(GCodeLine);
        GCodeLine[0] = '\0'; // Clear the buffer after sending
      }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
*/

void MarlinSender(const char* line) {
  while (!handshake.canSendNow()) {
    // Wait until it's safe to send the next command
    vTaskDelay(100 / portTICK_PERIOD_MS);
    handshake.processInput(); // Process any incoming responses from Marlin
  }
  handshake.sendLine(line);
  Serial.println("Sent G-code to Marlin:");
  Serial.println(line);
  vTaskDelay(100 / portTICK_PERIOD_MS);
}

bool sendGCodeFile(const char* filePath)
{
  if ((filePath == nullptr) || (filePath[0] == '\0'))
  {
    localDebug.println("G-code file path is empty");
    return false;
  }

  File gcodeFile = SPIFFS.open(filePath, FILE_READ);
  if (!gcodeFile || gcodeFile.isDirectory())
  {
    localDebug.println("Failed to open G-code file: " + String(filePath));
    return false;
  }

  localDebug.println("Sending G-code from file: " + String(filePath));

  char lineBuffer[128];
  size_t sentLines = 0;

  while (gcodeFile.available())
  {
    size_t len = gcodeFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
    lineBuffer[len] = '\0';

    while ((len > 0) && ((lineBuffer[len - 1] == '\r') || (lineBuffer[len - 1] == ' ') || (lineBuffer[len - 1] == '\t')))
    {
      lineBuffer[--len] = '\0';
    }

    size_t start = 0;
    while ((lineBuffer[start] == ' ') || (lineBuffer[start] == '\t'))
    {
      ++start;
    }

    char* line = &lineBuffer[start];
    if (line[0] == '\0')
    {
      continue;
    }

    if (line[0] == ';')
    {
      continue;
    }

    char* inlineComment = strchr(line, ';');
    if (inlineComment != nullptr)
    {
      *inlineComment = '\0';

      size_t trimmedLen = strlen(line);
      while ((trimmedLen > 0) && ((line[trimmedLen - 1] == ' ') || (line[trimmedLen - 1] == '\t')))
      {
        line[--trimmedLen] = '\0';
      }

      if (line[0] == '\0')
      {
        continue;
      }
    }

    MarlinSender(line);
    ++sentLines;
  }

  gcodeFile.close();
  localDebug.println("Finished G-code file send, lines sent: " + String(sentLines));
  return true;
}

bool sendGCodeFileList(const char* listFilePath)
{
  if ((listFilePath == nullptr) || (listFilePath[0] == '\0'))
  {
    localDebug.println("G-code list file path is empty");
    return false;
  }

  File listFile = SPIFFS.open(listFilePath, FILE_READ);
  if (!listFile || listFile.isDirectory())
  {
    localDebug.println("Failed to open G-code list file: " + String(listFilePath));
    return false;
  }

  localDebug.println("Sending G-code files from list: " + String(listFilePath));

  char pathBuffer[128];
  size_t fileCount = 0;
  bool allSucceeded = true;

  while (listFile.available())
  {
    size_t len = listFile.readBytesUntil('\n', pathBuffer, sizeof(pathBuffer) - 1);
    pathBuffer[len] = '\0';

    while ((len > 0) && ((pathBuffer[len - 1] == '\r') || (pathBuffer[len - 1] == ' ') || (pathBuffer[len - 1] == '\t')))
    {
      pathBuffer[--len] = '\0';
    }

    size_t start = 0;
    while ((pathBuffer[start] == ' ') || (pathBuffer[start] == '\t'))
    {
      ++start;
    }

    char* gcodePath = &pathBuffer[start];
    if (gcodePath[0] == '\0')
    {
      continue;
    }

    if (gcodePath[0] == ';')
    {
      continue;
    }

    char* inlineComment = strchr(gcodePath, ';');
    if (inlineComment != nullptr)
    {
      *inlineComment = '\0';

      size_t trimmedLen = strlen(gcodePath);
      while ((trimmedLen > 0) && ((gcodePath[trimmedLen - 1] == ' ') || (gcodePath[trimmedLen - 1] == '\t')))
      {
        gcodePath[--trimmedLen] = '\0';
      }

      if (gcodePath[0] == '\0')
      {
        continue;
      }
    }

    ++fileCount;
    if (!sendGCodeFile(gcodePath))
    {
      localDebug.println("Failed while sending listed file: " + String(gcodePath));
      allSucceeded = false;
    }
  }

  listFile.close();
  localDebug.println("Finished G-code list send, files processed: " + String(fileCount));
  return allSucceeded;
}

void setupUserCode() 
{
    // User code setup
    // This function is called during setup() in main.cpp and can be used to perform any user defined setup required for the action sequences, such as configuring GPIOs, initializing sensors, etc.
    // For example, to set the play and stop functions for Action 0 to the template functions defined below:

    // hardcode GPIO types here
    gcodeUart.begin(250000, SERIAL_8N1, 22, 23);  // baud, config, RX pin, TX pin

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

    strcpy(action[2].name, "GCode File Send");
    action[2].setActionPlayFunction(action2PlayFcn);
    action[2].setActionStopFunction(action2StopFcn);

    strcpy(action[3].name, "GCode List Send");
    action[3].setActionPlayFunction(action3PlayFcn);
    action[3].setActionStopFunction(action3StopFcn);

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
  float x = 100;
  float y = 100;
  float z = 90;
  float feedrate = 800;

  char GCodeLine[128]; // Buffer to hold the next G-code line to send to Marlin

  snprintf(GCodeLine, sizeof(GCodeLine), "G28 X%.3f Y%.3f Z%.3f F%.1f", x, y, z, feedrate);
  MarlinSender(GCodeLine); // Send the home command to Marlin
  localDebug.println("Sent G-code command to home X, Y and Z axes");

  x=100;
  y=100;
  snprintf(GCodeLine, sizeof(GCodeLine), "G1 X%.3f Y%.3f Z%.3f F%.1f", x, y, z, feedrate);
  MarlinSender(GCodeLine); // Send the move command to Marlin
  localDebug.println("Sent G-code command to move X, Y and Z axes");

  return(0);       // Done
}

int action1StopFcn(uint8_t number)
{
  action[number].stop(CMD_LOCAL);
  return(0);
}

int action2PlayFcn(uint8_t number)
{
  (void)number;
  const char* gcodeFilePath = "/job.gcode";

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
  const char* gcodeListPath = "/joblist.txt";

  if (!sendGCodeFileList(gcodeListPath))
  {
    localDebug.println("Action 3 completed with errors from list: " + String(gcodeListPath));
    return(0);
  }

  localDebug.println("Action 3 sent all files from list: " + String(gcodeListPath));
  return(0);
}

int action3StopFcn(uint8_t number)
{
  action[number].stop(CMD_LOCAL);
  return(0);
}
