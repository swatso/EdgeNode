#ifndef SCENE_CALLBACKS_H
#define SCENE_CALLBACKS_H

#include <Arduino.h>
#include "GCodeObjectConfig.h"

// Callback function signature: receives the scene index and execution result
typedef void (*SceneCompletionCallback)(uint8_t sceneIndex, bool success);

class SceneCallbackManager
{
public:
  // Register a callback to be executed when a specific scene completes
  // Returns true if successful, false if max callbacks for this scene reached
  bool registerCallback(uint8_t sceneIndex, SceneCompletionCallback callback);

  // Unregister all callbacks for a specific scene
  void unregisterAllCallbacks(uint8_t sceneIndex);

  // Unregister a specific callback (if registered multiple times, removes one instance)
  void unregisterCallback(uint8_t sceneIndex, SceneCompletionCallback callback);

  // Invoke all registered callbacks for a scene (called internally by scene execution)
  void invokeCallbacks(uint8_t sceneIndex, bool success);

  // Get the number of callbacks registered for a scene
  uint8_t getCallbackCount(uint8_t sceneIndex) const;

  // Clear all callbacks for all scenes
  void clear();

private:
  static constexpr uint8_t kMaxCallbacksPerScene = 4;
  
  struct SceneCallbackEntry
  {
    SceneCompletionCallback callbacks[kMaxCallbacksPerScene];
    uint8_t count;

    SceneCallbackEntry() : count(0) {
      for (uint8_t i = 0; i < kMaxCallbacksPerScene; ++i) {
        callbacks[i] = nullptr;
      }
    }
  };

  SceneCallbackEntry callbackRegistry[kSceneCount];
};

extern SceneCallbackManager sceneCallbacks;

#endif // SCENE_CALLBACKS_H
