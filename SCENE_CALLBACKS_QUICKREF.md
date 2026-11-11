# Scene Callbacks - Quick Reference

## Basic Usage

### 1. Write a callback function
```cpp
void mySceneCompletion(uint8_t sceneIndex, bool success)
{
  // 'success' is true if scene executed successfully, false otherwise
  localOperations.println(success ? "Scene OK" : "Scene Failed");
}
```

### 2. Register it in setupUserCode()
```cpp
void setupUserCode()
{
  sceneCallbacks.registerCallback(3, mySceneCompletion);
}
```

### 3. Queue your scene normally
```cpp
bool scene3RunFcn(uint8_t number)
{
  return queueSceneRun(number);  // Callback fires after this completes
}
```

## Common Patterns

### Pattern: Perform cleanup after scene
```cpp
void sceneCleanup(uint8_t sceneIndex, bool success)
{
  if (success)
    gpio[1].digitalWrite(LOW);  // Release lock
}

sceneCallbacks.registerCallback(3, sceneCleanup);
```

### Pattern: Trigger next scene on completion
```cpp
void triggerNextScene(uint8_t sceneIndex, bool success)
{
  if (success)
    queueSceneRun(4);  // Run scene 4 after scene 3 completes
}

sceneCallbacks.registerCallback(3, triggerNextScene);
```

### Pattern: Log results
```cpp
void logSceneResult(uint8_t sceneIndex, bool success)
{
  localDebug.printf("Scene %u: %s\n", sceneIndex, success ? "OK" : "FAIL");
}

sceneCallbacks.registerCallback(3, logSceneResult);
```

### Pattern: Play completion sound
```cpp
void playCompletionSound(uint8_t sceneIndex, bool success)
{
  mp3.play(CMD_LOCAL, success ? 1 : 4, false);
}

sceneCallbacks.registerCallback(3, playCompletionSound);
```

## API Quick Ref

| Function | Purpose |
|----------|---------|
| `sceneCallbacks.registerCallback(sceneIdx, fn)` | Register callback (max 4 per scene) |
| `sceneCallbacks.unregisterCallback(sceneIdx, fn)` | Unregister one callback |
| `sceneCallbacks.unregisterAllCallbacks(sceneIdx)` | Clear all callbacks for scene |
| `sceneCallbacks.getCallbackCount(sceneIdx)` | Check how many are registered |
| `sceneCallbacks.clear()` | Clear all callbacks everywhere |

## Callback Signature
```cpp
typedef void (*SceneCompletionCallback)(uint8_t sceneIndex, bool success);
```

## Key Points

✓ **Non-blocking** - Callbacks run after scene completes, user code continues  
✓ **Flexible** - Up to 4 callbacks per scene, execute sequentially  
✓ **Simple** - Just register in setup and write your handler function  
✓ **Reliable** - Only invoked when Marlin confirms completion  

⚠️ **Don't use `delay()`** in callbacks  
⚠️ **Don't block** waiting for long operations  
⚠️ **Do queue tasks** for expensive work instead  

## Example: Complete Setup

```cpp
// In UserCode.h
void scene3CompletionHandler(uint8_t sceneIndex, bool success);

// In UserCode.cpp
void scene3CompletionHandler(uint8_t sceneIndex, bool success)
{
  if (success)
  {
    localOperations.println("Scene 3 done!");
    gpio[1].digitalWrite(LOW);  // Release lock
  }
  else
  {
    localDebug.println("Scene 3 failed");
    mp3.play(CMD_LOCAL, 4, false);  // Error sound
  }
}

void setupUserCode()
{
  initGCodeControl();
  // ... other setup ...
  
  scenes[3].setRunFunction(scene3RunFcn);
  sceneCallbacks.registerCallback(3, scene3CompletionHandler);
  
  localDebug.println("User code setup completed");
}

bool scene3RunFcn(uint8_t number)
{
  mp3.play(CMD_LOCAL, 2, false);
  return queueSceneRun(number);
  // Callback fires automatically after scene completes!
}
```

## Multiple Callbacks Example

```cpp
void setupUserCode()
{
  // All three callbacks fire in order after scene 3 completes
  sceneCallbacks.registerCallback(3, logResults);
  sceneCallbacks.registerCallback(3, updateUI);
  sceneCallbacks.registerCallback(3, triggerNextScene);
}
```

## See Also

- [SCENE_CALLBACKS.md](./SCENE_CALLBACKS.md) - Full documentation
- [include/SceneCallbacks.h](./include/SceneCallbacks.h) - API header
- [src/SceneCallbacks.cpp](./src/SceneCallbacks.cpp) - Implementation
