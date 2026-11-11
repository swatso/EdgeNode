# Scene Completion Callbacks

## Overview

The Scene Callbacks system provides a clean, flexible mechanism to execute code after a scene completes execution. Instead of blocking the user function after `queueSceneRun()`, you can now register callback functions that will be invoked once the scene finishes running on the Marlin SD card.

## Architecture

### How It Works

1. **User calls `queueSceneRun(sceneIndex)`** → Scene execution is queued as a background task
2. **Scene executes on Marlin** → SD file runs asynchronously
3. **Scene completes** (success or failure) → Marlin reports completion via handshake
4. **Callbacks invoked** → All registered callbacks for that scene execute sequentially
5. **User code continues** → Post-completion actions can be performed in the callback

The callback mechanism runs in the `sceneRunTask`, which is a dedicated FreeRTOS task that processes scene execution. This ensures:
- **Non-blocking**: Your main MQTT and sensor code continues running
- **Sequential**: Callbacks execute in registration order
- **Deterministic**: Callbacks run after completion is confirmed by Marlin

## API Reference

### SceneCallbackManager (Global Instance: `sceneCallbacks`)

#### `bool registerCallback(uint8_t sceneIndex, SceneCompletionCallback callback)`

Registers a callback to be invoked when the specified scene completes.

**Parameters:**
- `sceneIndex`: Scene index (0-15)
- `callback`: Function pointer matching `void (*)(uint8_t sceneIndex, bool success)`

**Returns:** 
- `true` if registration successful
- `false` if scene index is invalid or maximum callbacks per scene (4) exceeded

**Example:**
```cpp
sceneCallbacks.registerCallback(3, scene3CompletionHandler);
```

#### `void unregisterCallback(uint8_t sceneIndex, SceneCompletionCallback callback)`

Unregisters a specific callback for a scene.

**Parameters:**
- `sceneIndex`: Scene index (0-15)
- `callback`: Function pointer to unregister

**Example:**
```cpp
sceneCallbacks.unregisterCallback(3, scene3CompletionHandler);
```

#### `void unregisterAllCallbacks(uint8_t sceneIndex)`

Unregisters all callbacks for a scene.

**Parameters:**
- `sceneIndex`: Scene index (0-15)

**Example:**
```cpp
sceneCallbacks.unregisterAllCallbacks(3);
```

#### `uint8_t getCallbackCount(uint8_t sceneIndex) const`

Returns the number of registered callbacks for a scene.

**Parameters:**
- `sceneIndex`: Scene index (0-15)

**Returns:** Number of registered callbacks (0-4)

**Example:**
```cpp
uint8_t count = sceneCallbacks.getCallbackCount(3);
```

#### `void clear()`

Clears all callbacks for all scenes.

**Example:**
```cpp
sceneCallbacks.clear();
```

## Usage Examples

### Example 1: Simple Post-Scene Cleanup

```cpp
// Callback function
void scene3CompletionHandler(uint8_t sceneIndex, bool success)
{
  if (success)
  {
    localOperations.println("Scene 3 completed successfully");
    // Perform cleanup or next action
    gpio[0].digitalWrite(LOW);  // Release a lock
  }
  else
  {
    localDebug.println("Scene 3 failed");
    // Handle failure
    mp3.play(CMD_LOCAL, 4, false);  // Play error sound
  }
}

void setupUserCode()
{
  sceneCallbacks.registerCallback(3, scene3CompletionHandler);
}

bool scene3RunFcn(uint8_t number)
{
  mp3.play(CMD_LOCAL, 2, false);
  return queueSceneRun(number);
}
```

### Example 2: Multiple Callbacks Per Scene

You can register up to 4 callbacks per scene. They execute sequentially in registration order:

```cpp
void sceneLogging(uint8_t sceneIndex, bool success)
{
  // Log scene execution
  localDebug.print("Scene ");
  localDebug.print(sceneIndex);
  localDebug.print(" result: ");
  localDebug.println(success);
}

void sceneUI(uint8_t sceneIndex, bool success)
{
  // Update UI or MQTT state
  if (success)
  {
    // Send completion message
  }
}

void sceneCleanup(uint8_t sceneIndex, bool success)
{
  // Perform cleanup
  mp3.play(CMD_LOCAL, 1, false);
}

void setupUserCode()
{
  // Register in execution order
  sceneCallbacks.registerCallback(3, sceneLogging);
  sceneCallbacks.registerCallback(3, sceneUI);
  sceneCallbacks.registerCallback(3, sceneCleanup);
}
```

### Example 3: Dynamic Callback Registration

Register callbacks based on runtime conditions:

```cpp
bool scene1RunFcn(uint8_t number)
{
  // Conditionally register different callbacks
  if (operatingMode == MODE_STANDARD)
  {
    sceneCallbacks.registerCallback(1, standardModeCompletion);
  }
  else if (operatingMode == MODE_MAINTENANCE)
  {
    sceneCallbacks.registerCallback(1, maintenanceModeCompletion);
  }
  
  return queueSceneRun(number);
}
```

### Example 4: Chaining Scenes

Execute one scene after another completes:

```cpp
void scene0CompletionHandler(uint8_t sceneIndex, bool success)
{
  if (success)
  {
    localOperations.println("Scene 0 done, starting scene 1");
    queueSceneRun(1);  // Start next scene
  }
}

void setupUserCode()
{
  sceneCallbacks.registerCallback(0, scene0CompletionHandler);
}
```

### Example 5: Sensor-Based Post-Scene Actions

Perform actions after a scene based on sensor readings:

```cpp
void sceneCompletionWithSensorCheck(uint8_t sceneIndex, bool success)
{
  if (!success) return;
  
  // Check pallet presence
  if (!gpio[0].digitalRead())  // Pallet present
  {
    localOperations.println("Pallet detected, proceeding");
    gpio[1].digitalWrite(HIGH);  // Engage lock
  }
  else
  {
    localDebug.println("Pallet missing, alert");
    mp3.play(CMD_LOCAL, 4, false);
  }
}
```

## Best Practices

### ✓ DO

- Keep callbacks **short and non-blocking**
  - They run in the scene task context
  - Avoid long operations; queue other tasks if needed

- Use callbacks for:
  - Logging and diagnostics
  - Updating state variables
  - Queuing the next scene or action
  - Sending MQTT messages
  - Simple GPIO operations
  - Playing sounds

- Register callbacks in `setupUserCode()`
  - Or dynamically register before calling `queueSceneRun()`

- Check the `success` parameter
  - Always handle both success and failure cases

### ✗ DON'T

- Block in callbacks with `delay()` or `while()` loops
  - This will freeze the scene task

- Perform expensive operations (file I/O, network requests)
  - Queue a separate task instead

- Call `Scene::run()` or scene-related functions recursively
  - Use `queueSceneRun()` to queue another scene instead

- Assume callback execution is immediate
  - Callbacks execute after Marlin reports completion
  - There's a small delay between SD print complete and callback invocation

## Implementation Details

### Callback Limits

- **Maximum callbacks per scene:** 4
- **Total scenes:** 16 (0-15)
- **Total possible callbacks:** 64

If you need more callbacks per scene, modify `kMaxCallbacksPerScene` in [SceneCallbacks.h](/include/SceneCallbacks.h).

### Thread Safety

The callback system is designed for single-threaded use within the scene task. The scene task is the only consumer of scene completion events, so no mutex is required. However:

- **Do not** call `registerCallback()` / `unregisterCallback()` from multiple threads
- **Do** call them from MQTT handlers or setup code (main task context)
- Callbacks themselves execute in the scene task context

### Memory Usage

```
Per scene:
  - Array of 4 function pointers: 16 bytes (4 pointers × 4 bytes each)
  - Callback count: 1 byte
  - Total per scene: ~17 bytes
  - Total for all 16 scenes: ~272 bytes
```

## Integration Points

The callback system integrates at [WiFiManager.cpp:sceneRunTask()](/src/WiFiManager.cpp) where `sceneCallbacks.invokeCallbacks()` is called after `runScene()` completes.

## Files Modified/Created

- **Created:** [include/SceneCallbacks.h](/include/SceneCallbacks.h) - Header with API and SceneCallbackManager class
- **Created:** [src/SceneCallbacks.cpp](/src/SceneCallbacks.cpp) - Implementation
- **Modified:** [src/WiFiManager.cpp](/src/WiFiManager.cpp) - Added callback invocation in sceneRunTask()
- **Modified:** [src/UserCode.cpp](/src/UserCode.cpp) - Added example callback usage
- **Modified:** [include/UserCode.h](/include/UserCode.h) - Added callback declaration

## Troubleshooting

### Callback Not Being Called

1. **Verify registration:** Check that `sceneCallbacks.registerCallback()` returned `true`
2. **Verify scene ran:** Confirm the scene index is correct and `queueSceneRun()` was called
3. **Check success parameter:** Even if scene failed, callback should still be called
4. **Debug output:** Add `localDebug.println()` in your callback to verify it's being reached

### Multiple Callback Registrations

If you accidentally register the same callback multiple times, only one instance should fire per scene completion. To avoid duplicates, call `unregisterCallback()` before re-registering:

```cpp
sceneCallbacks.unregisterCallback(3, scene3Handler);
sceneCallbacks.registerCallback(3, scene3Handler);
```

### Callback Memory Issues

If memory is constrained, reduce callbacks per scene or defer post-scene actions to MQTT handlers instead of callbacks.
