# Scene Completion Callbacks - Implementation Summary

## Overview

I've implemented a robust callback mechanism that allows user-defined code to execute after a scene completes on the Marlin SD card. This solves your requirement for post-scene actions without blocking the user function after `queueSceneRun()`.

## What Was Built

### 1. **SceneCallbackManager Class** ([include/SceneCallbacks.h](/include/SceneCallbacks.h))

A global singleton that manages scene completion callbacks with the following capabilities:

- **Register callbacks**: `registerCallback(sceneIndex, callback)` - max 4 per scene
- **Unregister callbacks**: `unregisterCallback()` or `unregisterAllCallbacks()`
- **Query callbacks**: `getCallbackCount()` to check registration status
- **Invoke callbacks**: `invokeCallbacks(sceneIndex, success)` - called automatically

**Key Features:**
- Type-safe function pointers with clear signature: `void (*)(uint8_t sceneIndex, bool success)`
- Per-scene callback arrays with bounds checking
- Supports multiple callbacks per scene (executed sequentially)
- Zero mutex overhead (single-threaded scene task context)

### 2. **Implementation** ([src/SceneCallbacks.cpp](/src/SceneCallbacks.cpp))

Full implementation of the SceneCallbackManager with:
- Safe registration with return status
- Proper array management and bounds checking
- Sequential callback invocation for a scene
- Clean-up methods for unregistering individual or all callbacks

### 3. **Integration with Scene Execution** ([src/WiFiManager.cpp](/src/WiFiManager.cpp))

Modified `sceneRunTask()` to:
1. Execute `runScene(request.index)` and capture result
2. **Automatically invoke all registered callbacks** via `sceneCallbacks.invokeCallbacks(index, success)`
3. Run non-blocking in the background scene task

**The Flow:**
```
MQTT/User Code calls queueSceneRun(3)
    ↓
Scene task receives request
    ↓
runScene(3) executes → Marlin runs SD file
    ↓
Scene completes on SD card (Marlin reports "Done printing file")
    ↓
Callback invocation: all callbacks for scene 3 fire with success status
    ↓
Scene task waits for next scene request
```

### 4. **User-Facing API Updates** 

#### [include/UserCode.h](/include/UserCode.h)
- Added callback function declaration: `void scene3CompletionHandler(...)`

#### [src/UserCode.cpp](/src/UserCode.cpp)
- Added example callback implementation
- Updated `setupUserCode()` to show callback registration
- Enhanced documentation with callback usage instructions

## Usage Example

### Before (Blocking Approach - Not Ideal)
```cpp
bool scene3RunFcn(uint8_t number)
{
  mp3.play(CMD_LOCAL, 2, false);
  
  // This doesn't work well because queueSceneRun() is async
  // and we can't reliably know when it's done
  queueSceneRun(number);
  
  // Can't perform post-scene actions here!
  return true;
}
```

### After (Callback Approach - Recommended)
```cpp
// 1. Write your callback
void scene3CompletionHandler(uint8_t sceneIndex, bool success)
{
  if (success)
  {
    localOperations.println("Scene 3 completed!");
    gpio[1].digitalWrite(LOW);  // Release lock
  }
  else
  {
    mp3.play(CMD_LOCAL, 4, false);  // Error sound
  }
}

// 2. Register it
void setupUserCode()
{
  sceneCallbacks.registerCallback(3, scene3CompletionHandler);
}

// 3. Queue scene normally
bool scene3RunFcn(uint8_t number)
{
  mp3.play(CMD_LOCAL, 2, false);
  return queueSceneRun(number);
  // Callback automatically fires when scene completes!
}
```

## Architecture Highlights

### Non-Blocking Design
- `queueSceneRun()` returns immediately
- Scene execution runs asynchronously in the scene task
- Callbacks fire after Marlin confirms completion
- User code and other tasks continue running

### Thread-Safe Single-Threaded Context
- All scene operations happen in `sceneRunTask()`
- No mutex required (single consumer)
- Safe to register callbacks from MQTT handlers or setup code
- Callbacks execute in the scene task context

### Scalable Callback Support
- Up to 4 callbacks per scene (16 total scenes = max 64 callbacks)
- Sequential execution in registration order
- Individual or bulk unregistration
- Configurable limit (just change `kMaxCallbacksPerScene`)

### Minimal Memory Footprint
- ~272 bytes total for all 16 scenes × 4 callbacks
- Callback pointers only, no additional data storage
- No dynamic allocation (static registry)

## Files Modified/Created

| File | Change | Purpose |
|------|--------|---------|
| [include/SceneCallbacks.h](/include/SceneCallbacks.h) | **Created** | API header and SceneCallbackManager class |
| [src/SceneCallbacks.cpp](/src/SceneCallbacks.cpp) | **Created** | Full implementation |
| [src/WiFiManager.cpp](/src/WiFiManager.cpp) | **Modified** | Added callback invocation in sceneRunTask() |
| [src/UserCode.cpp](/src/UserCode.cpp) | **Modified** | Added example callback and registration |
| [include/UserCode.h](/include/UserCode.h) | **Modified** | Added callback function declaration |
| [SCENE_CALLBACKS.md](/SCENE_CALLBACKS.md) | **Created** | Complete API documentation |
| [SCENE_CALLBACKS_QUICKREF.md](/SCENE_CALLBACKS_QUICKREF.md) | **Created** | Quick reference guide |

## Verification

✅ **Build Status:** Project compiles successfully with `-e target1`  
✅ **Integration:** Callbacks properly integrated with scene execution flow  
✅ **No Breaking Changes:** Existing scene functions work unchanged  
✅ **Backward Compatible:** Scenes without callbacks work as before  

## Advanced Usage

### Multiple Callbacks (Chaining)
```cpp
void setupUserCode()
{
  // All three fire in order
  sceneCallbacks.registerCallback(3, logResults);
  sceneCallbacks.registerCallback(3, updateUI);
  sceneCallbacks.registerCallback(3, triggerNextScene);
}
```

### Dynamic Registration
```cpp
bool scene1RunFcn(uint8_t number)
{
  // Register different callbacks based on runtime state
  if (maintenanceMode)
    sceneCallbacks.registerCallback(1, maintenanceCallback);
  else
    sceneCallbacks.registerCallback(1, normalCallback);
  
  return queueSceneRun(number);
}
```

### Scene Sequencing
```cpp
void sceneSequenceHandler(uint8_t sceneIndex, bool success)
{
  if (success && sceneIndex == 2)
  {
    queueSceneRun(3);  // Automatically run next scene
  }
}

void setupUserCode()
{
  sceneCallbacks.registerCallback(2, sceneSequenceHandler);
}
```

## Best Practices

✅ **Keep callbacks short** - They run in the scene task  
✅ **Handle both success and failure** - Check the `success` parameter  
✅ **Queue async work** - Don't block with `delay()` or loops  
✅ **Register in setup** - Or register before `queueSceneRun()`  

❌ **Don't use `delay()`** - Will freeze the scene task  
❌ **Don't do expensive I/O** - Queue a separate task instead  
❌ **Don't call `Scene::run()`** - Use `queueSceneRun()` instead  

## Next Steps for Your Code

1. **Review** the examples in [SCENE_CALLBACKS_QUICKREF.md](/SCENE_CALLBACKS_QUICKREF.md)
2. **Implement** your callback functions for each scene
3. **Register** callbacks in `setupUserCode()`
4. **Test** by running scenes and verifying callback execution
5. **Extend** with multiple callbacks per scene as needed

## Support for Custom Needs

If you need more callbacks per scene, modify `kMaxCallbacksPerScene` in [SceneCallbacks.h](/include/SceneCallbacks.h):

```cpp
static constexpr uint8_t kMaxCallbacksPerScene = 8;  // Increase from 4
```

Then rebuild. The implementation automatically scales.

## Summary

This implementation provides a clean, efficient mechanism for post-scene actions while maintaining:
- **Non-blocking execution** - No delays in your main code
- **Type safety** - Compile-time function pointer validation
- **Simplicity** - Easy-to-use API with clear semantics
- **Flexibility** - Support for multiple callbacks and dynamic registration
- **Performance** - Minimal overhead, no dynamic allocation

Your user code can now cleanly handle post-scene logic without workarounds!
