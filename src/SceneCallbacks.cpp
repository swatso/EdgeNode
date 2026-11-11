#include "SceneCallbacks.h"

SceneCallbackManager sceneCallbacks;

bool SceneCallbackManager::registerCallback(uint8_t sceneIndex, SceneCompletionCallback callback)
{
  if ((sceneIndex >= kSceneCount) || (callback == nullptr))
  {
    return false;
  }

  SceneCallbackEntry& entry = callbackRegistry[sceneIndex];
  
  // Check if we have space for another callback
  if (entry.count >= kMaxCallbacksPerScene)
  {
    return false;
  }

  // Add the callback
  entry.callbacks[entry.count] = callback;
  entry.count++;

  return true;
}

void SceneCallbackManager::unregisterAllCallbacks(uint8_t sceneIndex)
{
  if (sceneIndex >= kSceneCount)
  {
    return;
  }

  SceneCallbackEntry& entry = callbackRegistry[sceneIndex];
  for (uint8_t i = 0; i < kMaxCallbacksPerScene; ++i)
  {
    entry.callbacks[i] = nullptr;
  }
  entry.count = 0;
}

void SceneCallbackManager::unregisterCallback(uint8_t sceneIndex, SceneCompletionCallback callback)
{
  if ((sceneIndex >= kSceneCount) || (callback == nullptr))
  {
    return;
  }

  SceneCallbackEntry& entry = callbackRegistry[sceneIndex];
  
  // Find and remove the first matching callback
  for (uint8_t i = 0; i < entry.count; ++i)
  {
    if (entry.callbacks[i] == callback)
    {
      // Shift remaining callbacks down
      for (uint8_t j = i; j < (entry.count - 1); ++j)
      {
        entry.callbacks[j] = entry.callbacks[j + 1];
      }
      entry.callbacks[entry.count - 1] = nullptr;
      entry.count--;
      return;
    }
  }
}

void SceneCallbackManager::invokeCallbacks(uint8_t sceneIndex, bool success)
{
  if (sceneIndex >= kSceneCount)
  {
    return;
  }

  SceneCallbackEntry& entry = callbackRegistry[sceneIndex];
  
  // Invoke all registered callbacks for this scene
  for (uint8_t i = 0; i < entry.count; ++i)
  {
    if (entry.callbacks[i] != nullptr)
    {
      entry.callbacks[i](sceneIndex, success);
    }
  }
}

uint8_t SceneCallbackManager::getCallbackCount(uint8_t sceneIndex) const
{
  if (sceneIndex >= kSceneCount)
  {
    return 0;
  }

  return callbackRegistry[sceneIndex].count;
}

void SceneCallbackManager::clear()
{
  for (uint8_t i = 0; i < kSceneCount; ++i)
  {
    unregisterAllCallbacks(i);
  }
}
