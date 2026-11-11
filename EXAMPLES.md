# Scene Callbacks - Real-World Examples

This document shows practical examples of how to use scene callbacks for common operations.

## Example 1: Machine Lock Control

**Scenario:** Release a mechanical lock after a scene completes

```cpp
// UserCode.h
void lockReleaseHandler(uint8_t sceneIndex, bool success);

// UserCode.cpp
void lockReleaseHandler(uint8_t sceneIndex, bool success)
{
  if (!success)
  {
    localDebug.println("Scene failed, lock NOT released");
    return;
  }

  // Release the lock by pulling GPIO low
  gpio[1].digitalWrite(LOW);
  localOperations.println("Lock released");
}

void setupUserCode()
{
  gpio[1].setType(GPIO_DIGOUT);
  strcpy(gpio[1].name, "Lock");
  
  sceneCallbacks.registerCallback(3, lockReleaseHandler);
}

bool scene3RunFcn(uint8_t number)
{
  // Engage lock before scene
  gpio[1].digitalWrite(HIGH);
  localOperations.println("Lock engaged");
  
  return queueSceneRun(number);
  // Lock automatically releases when scene completes!
}
```

## Example 2: Scene Sequencing with Feedback

**Scenario:** Run a multi-step process where each scene triggers the next

```cpp
// UserCode.h
void sceneSequenceHandler(uint8_t sceneIndex, bool success);

// UserCode.cpp
void sceneSequenceHandler(uint8_t sceneIndex, bool success)
{
  if (!success)
  {
    localDebug.print("Scene ");
    localDebug.print(sceneIndex);
    localDebug.println(" failed! Sequence aborted.");
    mp3.play(CMD_LOCAL, 4, false);  // Error sound
    return;
  }

  localOperations.print("Scene ");
  localOperations.print(sceneIndex);
  localOperations.println(" completed successfully");
  
  mp3.play(CMD_LOCAL, 1, false);  // Success sound
  
  // Trigger next scene in sequence
  uint8_t nextScene = sceneIndex + 1;
  if (nextScene <= 5)  // Sequence goes from scene 1 to 5
  {
    localOperations.print("Starting scene ");
    localOperations.println(nextScene);
    queueSceneRun(nextScene);
  }
  else
  {
    localOperations.println("All scenes completed!");
  }
}

void setupUserCode()
{
  // Register callback for all scenes in the sequence
  for (uint8_t i = 1; i <= 5; ++i)
  {
    sceneCallbacks.registerCallback(i, sceneSequenceHandler);
  }
}

bool scene1RunFcn(uint8_t number)
{
  localOperations.println("Starting scene sequence...");
  mp3.play(CMD_LOCAL, 2, false);  // Start sound
  return queueSceneRun(number);
  // Callback will trigger scene 2, which triggers scene 3, etc.
}
```

## Example 3: Sensor Validation After Scene

**Scenario:** Check sensors after a scene to verify operation completed

```cpp
// UserCode.h
void validatePalletPresence(uint8_t sceneIndex, bool success);

// UserCode.cpp
void validatePalletPresence(uint8_t sceneIndex, bool success)
{
  if (!success)
  {
    localDebug.println("Scene failed, skipping validation");
    return;
  }

  // Small delay to allow mechanical settling
  vTaskDelay(pdMS_TO_TICKS(500));

  // Check if pallet is now present (active low logic)
  bool palletPresent = (gpio[0].digitalRead() == LOW);

  if (palletPresent)
  {
    localOperations.println("✓ Pallet detected at destination");
    gpio[1].digitalWrite(HIGH);  // Engage lock
  }
  else
  {
    localDebug.println("✗ ERROR: Pallet not found!");
    mp3.play(CMD_LOCAL, 4, false);  // Error sound
    // Could queue a recovery scene here
  }
}

void setupUserCode()
{
  gpio[0].setType(GPIO_DIGIN);
  strcpy(gpio[0].name, "Pallet Detector");
  
  gpio[1].setType(GPIO_DIGOUT);
  strcpy(gpio[1].name, "Lock");
  
  sceneCallbacks.registerCallback(3, validatePalletPresence);
}
```

## Example 4: MQTT Status Updates

**Scenario:** Send operation status to MQTT broker after scene completes

```cpp
// UserCode.h
void sceneStatusUpdate(uint8_t sceneIndex, bool success);

// UserCode.cpp
void sceneStatusUpdate(uint8_t sceneIndex, bool success)
{
  // Build MQTT message
  MQTTMessagePayload msg;
  msg.type = MQTT_MSG_GCODE_STATE;
  
  // Scene index
  msg.gcode.scene = sceneIndex;
  
  // Status
  strcpy(msg.gcode.state, success ? "COMPLETE" : "FAILED");
  
  // Timestamp
  msg.gcode.ackDelay = 0;
  
  // Publish
  localOperations.print("Scene ");
  localOperations.print(sceneIndex);
  localOperations.print(" status: ");
  localOperations.println(msg.gcode.state);

  if (!publishMessage(msg))
  {
    localDebug.println("Failed to publish status update");
  }
}

void setupUserCode()
{
  // Register for all scenes
  for (uint8_t i = 0; i < 16; ++i)
  {
    sceneCallbacks.registerCallback(i, sceneStatusUpdate);
  }
}
```

## Example 5: Multi-Step Callback Chain

**Scenario:** Perform logging, validation, and trigger next action in sequence

```cpp
// UserCode.h
void scene2OnComplete_Log(uint8_t sceneIndex, bool success);
void scene2OnComplete_Validate(uint8_t sceneIndex, bool success);
void scene2OnComplete_Trigger(uint8_t sceneIndex, bool success);

// UserCode.cpp
void scene2OnComplete_Log(uint8_t sceneIndex, bool success)
{
  // Callback #1: Logging
  localDebug.printf("[Scene %u] Execution: %s\n", 
    sceneIndex, 
    success ? "SUCCESS" : "FAILURE");
}

void scene2OnComplete_Validate(uint8_t sceneIndex, bool success)
{
  // Callback #2: Validation
  if (success)
  {
    bool palletOk = (gpio[0].digitalRead() == LOW);
    bool lockOk = (gpio[3].digitalRead() == HIGH);
    
    if (palletOk && lockOk)
    {
      localOperations.println("All validations passed");
    }
    else
    {
      localDebug.println("Validation failed!");
    }
  }
}

void scene2OnComplete_Trigger(uint8_t sceneIndex, bool success)
{
  // Callback #3: Trigger next step
  if (success)
  {
    // Queue the next scene
    queueSceneRun(3);
    localOperations.println("Scene 3 queued");
  }
  else
  {
    // Go to error recovery scene
    queueSceneRun(15);  // Error handler scene
    localDebug.println("Error recovery initiated");
  }
}

void setupUserCode()
{
  // Register all three callbacks in order
  sceneCallbacks.registerCallback(2, scene2OnComplete_Log);
  sceneCallbacks.registerCallback(2, scene2OnComplete_Validate);
  sceneCallbacks.registerCallback(2, scene2OnComplete_Trigger);
  
  localDebug.println("Registered 3 callbacks for Scene 2");
}
```

## Example 6: Dynamic Callback Registration

**Scenario:** Register different callbacks based on operation mode

```cpp
// Global mode variable
enum OperationMode { MODE_NORMAL, MODE_MAINTENANCE, MODE_DIAGNOSTICS };
OperationMode currentMode = MODE_NORMAL;

// UserCode.h
void normalModeHandler(uint8_t sceneIndex, bool success);
void maintenanceModeHandler(uint8_t sceneIndex, bool success);
void diagnosticsModeHandler(uint8_t sceneIndex, bool success);

// UserCode.cpp
void normalModeHandler(uint8_t sceneIndex, bool success)
{
  if (success)
    queueSceneRun(sceneIndex + 1);  // Continue sequence
}

void maintenanceModeHandler(uint8_t sceneIndex, bool success)
{
  localOperations.println("MAINTENANCE MODE - Scene paused for inspection");
  mp3.play(CMD_LOCAL, 5, false);  // Beep for attention
  // Wait for manual intervention before continuing
}

void diagnosticsModeHandler(uint8_t sceneIndex, bool success)
{
  localDebug.printf("DIAGNOSTICS: Scene %u result=%s\n",
    sceneIndex, success ? "OK" : "FAIL");
  
  // Detailed sensor logging
  localDebug.printf("  Pallet: %s\n", 
    gpio[0].digitalRead() ? "absent" : "present");
  localDebug.printf("  Lock: %s\n", 
    gpio[3].digitalRead() ? "engaged" : "released");
}

bool scene3RunFcn(uint8_t number)
{
  // Unregister any previously registered callback
  sceneCallbacks.unregisterCallback(3, normalModeHandler);
  sceneCallbacks.unregisterCallback(3, maintenanceModeHandler);
  sceneCallbacks.unregisterCallback(3, diagnosticsModeHandler);
  
  // Register the appropriate callback based on current mode
  switch (currentMode)
  {
    case MODE_NORMAL:
      sceneCallbacks.registerCallback(3, normalModeHandler);
      break;
    case MODE_MAINTENANCE:
      sceneCallbacks.registerCallback(3, maintenanceModeHandler);
      break;
    case MODE_DIAGNOSTICS:
      sceneCallbacks.registerCallback(3, diagnosticsModeHandler);
      break;
  }
  
  return queueSceneRun(number);
}
```

## Example 7: Performance Monitoring

**Scenario:** Track scene execution timing and performance

```cpp
// Global timing array
struct ScenePerformance {
  uint32_t startTime;
  uint32_t endTime;
  uint32_t duration;
} scenePerf[16];

// UserCode.h
void performanceMonitor(uint8_t sceneIndex, bool success);
bool scene4RunFcn(uint8_t number);

// UserCode.cpp
void performanceMonitor(uint8_t sceneIndex, bool success)
{
  scenePerf[sceneIndex].endTime = millis();
  scenePerf[sceneIndex].duration = 
    scenePerf[sceneIndex].endTime - scenePerf[sceneIndex].startTime;

  localOperations.printf("Scene %u: %u ms (%s)\n",
    sceneIndex,
    (unsigned)scenePerf[sceneIndex].duration,
    success ? "OK" : "FAIL");

  if (scenePerf[sceneIndex].duration > 30000)
  {
    localDebug.println("WARNING: Slow scene execution!");
  }
}

bool scene4RunFcn(uint8_t number)
{
  scenePerf[4].startTime = millis();
  return queueSceneRun(number);
}

void setupUserCode()
{
  sceneCallbacks.registerCallback(4, performanceMonitor);
}
```

## Example 8: Error Recovery

**Scenario:** Implement automatic retry or recovery on scene failure

```cpp
// Global retry counter
uint8_t sceneRetryCount[16] = {0};
const uint8_t MAX_RETRIES = 3;

// UserCode.h
void sceneErrorRecovery(uint8_t sceneIndex, bool success);

// UserCode.cpp
void sceneErrorRecovery(uint8_t sceneIndex, bool success)
{
  if (success)
  {
    // Reset retry counter on success
    sceneRetryCount[sceneIndex] = 0;
    
    // Proceed to next step
    if (sceneIndex < 10)
      queueSceneRun(sceneIndex + 1);
  }
  else
  {
    // Scene failed
    sceneRetryCount[sceneIndex]++;
    
    localDebug.printf("Scene %u failed (attempt %u of %u)\n",
      sceneIndex,
      (unsigned)sceneRetryCount[sceneIndex],
      (unsigned)MAX_RETRIES);

    if (sceneRetryCount[sceneIndex] < MAX_RETRIES)
    {
      // Retry the same scene
      localOperations.println("Retrying scene...");
      vTaskDelay(pdMS_TO_TICKS(2000));  // Wait 2 seconds
      queueSceneRun(sceneIndex);
    }
    else
    {
      // Give up and go to error handler
      localDebug.println("Max retries exceeded, entering error state");
      sceneRetryCount[sceneIndex] = 0;
      mp3.play(CMD_LOCAL, 4, false);  // Error sound
      // Could queue error handler scene here
    }
  }
}

void setupUserCode()
{
  // Register for scenes that need error recovery
  for (uint8_t i = 1; i <= 5; ++i)
  {
    sceneCallbacks.registerCallback(i, sceneErrorRecovery);
  }
}
```

## Common Pitfalls and Solutions

### Pitfall 1: Blocking in Callbacks
```cpp
// ✗ BAD: Blocks the scene task!
void badCallback(uint8_t sceneIndex, bool success)
{
  delay(5000);  // Freezes scene task!
  gpio[1].digitalWrite(LOW);
}

// ✓ GOOD: Non-blocking
void goodCallback(uint8_t sceneIndex, bool success)
{
  gpio[1].digitalWrite(LOW);  // Returns immediately
}
```

### Pitfall 2: Forgetting Success Check
```cpp
// ✗ BAD: Processes even on failure
void badCallback(uint8_t sceneIndex, bool success)
{
  queueSceneRun(sceneIndex + 1);  // Wrong if scene failed!
}

// ✓ GOOD: Checks success first
void goodCallback(uint8_t sceneIndex, bool success)
{
  if (success)
    queueSceneRun(sceneIndex + 1);
  else
    localDebug.println("Scene failed");
}
```

### Pitfall 3: Not Handling Max Callbacks
```cpp
// ✗ BAD: Silently fails if >4 callbacks
void setupUserCode()
{
  sceneCallbacks.registerCallback(1, cb1);
  sceneCallbacks.registerCallback(1, cb2);
  sceneCallbacks.registerCallback(1, cb3);
  sceneCallbacks.registerCallback(1, cb4);
  sceneCallbacks.registerCallback(1, cb5);  // This one silently fails!
}

// ✓ GOOD: Check return status
void setupUserCode()
{
  if (!sceneCallbacks.registerCallback(1, cb5))
  {
    localDebug.println("ERROR: Too many callbacks!");
  }
}
```

## Testing Your Callbacks

```cpp
// Add to setupUserCode() temporarily
void debugCallback(uint8_t sceneIndex, bool success)
{
  localDebug.printf("DEBUG: Scene %u, success=%d\n", sceneIndex, success);
}

void setupUserCode()
{
  // Test your scene
  sceneCallbacks.registerCallback(3, debugCallback);
  localDebug.printf("Registered callbacks: %u\n", 
    sceneCallbacks.getCallbackCount(3));
}
```

Then run scene 3 and verify the debug message appears.
