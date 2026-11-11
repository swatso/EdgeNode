# Scene Callbacks - Architecture Diagram

## Execution Flow

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         MQTT/User Code                                   │
│                                                                          │
│  bool scene3RunFcn(uint8_t number)                                      │
│  {                                                                       │
│    mp3.play(CMD_LOCAL, 2, false);                                       │
│    return queueSceneRun(number);  ◄─── Queue scene 3 for async exec     │
│  }                                                                       │
│                                                                          │
│  ✓ Returns IMMEDIATELY                                                  │
│  ✓ Main code continues running                                          │
│  ✓ MQTT handlers can keep processing                                    │
└──────────────────────┬──────────────────────────────────────────────────┘
                       │
                       │ Scene request queued
                       │
                       ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                 Scene Task (FreeRTOS Task)                              │
│         "sceneRunTask" - Processes scene execution queue                │
│                                                                          │
│  for (;;) {                                                             │
│    xQueueReceive(sceneRunQueue, &request, ...)  ◄─── Wait for scene    │
│                                                                          │
│    ╔════════════════════════════════════════╗                          │
│    ║ bool success = runScene(request.index) ║  Execute scene via       │
│    ║                                        ║  Marlin handshake        │
│    ║ (runs GCode file from SD card)         ║                          │
│    ╚════════════════════════════════════════╝                          │
│               │                                                          │
│               │ Scene completes (Marlin reports done)                  │
│               │                                                          │
│               ▼                                                          │
│    ╔════════════════════════════════════════╗                          │
│    ║ sceneCallbacks.invokeCallbacks(        ║  Invoke ALL registered  │
│    ║   sceneIndex,                          ║  callbacks for this     │
│    ║   success  ◄─── Pass execution result  ║  scene sequentially     │
│    ║ )                                      ║                          │
│    ╚════════════════════════════════════════╝                          │
│  }                                                                       │
│                                                                          │
└──────────────────┬───────────────────────────────────────────────────────┘
                   │
                   │ For each registered callback:
                   │
                   ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                    Callback Execution                                    │
│                                                                          │
│  Callback #1: logSceneResult(sceneIndex=3, success=true)               │
│    → localDebug.printf("Scene 3: OK\n");                               │
│    → Returns immediately                                                │
│                                                                          │
│  Callback #2: updateUI(sceneIndex=3, success=true)                     │
│    → Send MQTT completion message                                       │
│    → Returns immediately                                                │
│                                                                          │
│  Callback #3: triggerNextScene(sceneIndex=3, success=true)             │
│    → queueSceneRun(4);  ◄─── Queue next scene                          │
│    → Returns immediately                                                │
│                                                                          │
│  All callbacks execute SEQUENTIALLY in registration order              │
│  Total execution time: < 10ms typically                                │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

## Memory Layout

```
SceneCallbackManager (Global Singleton)
│
└─ callbackRegistry[16]  ◄─── Array of callback entries (one per scene)
   │
   ├─ [0] Scene 0 entry
   │  ├─ callbacks[0..3]: 4 function pointers (or NULL)
   │  └─ count: 1 byte (number of registered callbacks)
   │
   ├─ [1] Scene 1 entry
   │  └─ callbacks[0..3], count
   │
   ├─ ...
   │
   └─ [15] Scene 15 entry
      └─ callbacks[0..3], count

Total: 16 scenes × 17 bytes = 272 bytes
```

## Callback Registry State Example

```
After setupUserCode() registers callbacks:

sceneCallbacks.registerCallback(3, scene3Logging);
sceneCallbacks.registerCallback(3, scene3UI);
sceneCallbacks.registerCallback(3, scene3Cleanup);

┌──────────────────────────────────────────┐
│ Scene 0: count=0, callbacks=[NULL, ...]  │
│ Scene 1: count=0, callbacks=[NULL, ...]  │
│ Scene 2: count=0, callbacks=[NULL, ...]  │
│                                          │
│ Scene 3: count=3                         │  ◄─── Our registered scene
│   callbacks[0] = scene3Logging           │
│   callbacks[1] = scene3UI                │
│   callbacks[2] = scene3Cleanup           │
│   callbacks[3] = NULL                    │
│                                          │
│ Scene 4: count=0, callbacks=[NULL, ...]  │
│ ...                                      │
│ Scene 15: count=0, callbacks=[NULL, ...] │
└──────────────────────────────────────────┘
```

## Callback Invocation Sequence

```
sceneCallbacks.invokeCallbacks(3, true)  ◄─── Called from sceneRunTask
│
├─► Entry: callbackRegistry[3]
│   ├─ count = 3
│   └─ callbacks[] = [scene3Logging, scene3UI, scene3Cleanup, NULL]
│
├─ For i = 0 to (count-1):
│  │
│  ├─ [i=0] Call scene3Logging(3, true)
│  │         │
│  │         └─► localDebug.printf("Scene 3: OK")
│  │             Returns immediately
│  │
│  ├─ [i=1] Call scene3UI(3, true)
│  │         │
│  │         └─► Send MQTT message
│  │             Returns immediately
│  │
│  └─ [i=2] Call scene3Cleanup(3, true)
│           │
│           └─► gpio[1].digitalWrite(LOW)
│               Returns immediately
│
└─ All callbacks complete, scene task waits for next request
```

## Integration Points

```
Main Application Flow:

User/MQTT Code
    │
    ├─ calls queueSceneRun(3) ──────┐
    │                               │
    │                               │
    └─ continues (non-blocking)     │
                                    │
                    ▼
            Scene Task Queue
                    │
                    │ (async, background)
                    │
                    ▼
    ┌───────────────────────────┐
    │ sceneRunTask() ★ KEY FILE │  [WiFiManager.cpp]
    │                           │
    │ 1. Wait for scene request │
    │ 2. Execute: runScene()    │
    │ 3. ★ NEW: Call callbacks  │  ◄─── We added this!
    │ 4. Loop back to step 1    │
    └───────────────────────────┘
                    │
                    │ After scene completes:
                    │
                    ▼
        SceneCallbackManager
        (invoke registered callbacks)
                    │
                    ├─► callback1(3, success)
                    ├─► callback2(3, success)
                    └─► callback3(3, success)
```

## Data Flow for Post-Scene Actions

```
Use Case: Lock releases and sound plays after scene finishes

┌─────────────────────────────────────────────────────────────┐
│ User calls:  queueSceneRun(3)                              │
│              (No waiting!)                                  │
└────────┬────────────────────────────────────────────────────┘
         │
         │  Background (Scene Task)
         │
         ├─→ Marlin executes scene 3 GCode from SD card
         │   • Moves robot
         │   • Performs machine actions
         │   • Takes ~5-60 seconds
         │
         ├─→ Marlin reports "Done printing file"
         │   • Handshake receives completion
         │   • runScene() returns with status
         │
         └─→ sceneCallbacks.invokeCallbacks(3, true)
             │
             ├─→ callback1: Log completion
             │   └─ 0.1ms
             │
             ├─→ callback2: Release lock
             │   └─ 0.2ms
             │   └─ gpio[1].digitalWrite(LOW)
             │
             └─→ callback3: Play completion sound
                 └─ 0.5ms
                 └─ mp3.play(CMD_LOCAL, 1, false)
                 
Total callback time: ~1ms (overlapped with next iteration)
```

## Comparison: Before vs After

```
BEFORE: No Post-Scene Callbacks
═══════════════════════════════════════════════════════════════

bool scene3RunFcn(uint8_t number)
{
  queueSceneRun(number);
  // ✗ Can't know when scene completes
  // ✗ Can't reliably perform post-scene actions
  // ✗ No way to handle failure
  return true;
}

Problem: No clean way to handle post-scene logic!


AFTER: With Post-Scene Callbacks
═══════════════════════════════════════════════════════════════

bool scene3RunFcn(uint8_t number)
{
  return queueSceneRun(number);  // Queue it
  // ✓ Returns immediately
  // ✓ Main code continues
}

void scene3CompletionHandler(uint8_t sceneIndex, bool success)
{
  // ✓ Called AFTER scene completes (Marlin confirms done)
  // ✓ Knows execution status (success/failure)
  // ✓ Can safely perform cleanup, trigger next scene, etc.
  
  if (success)
    queueSceneRun(4);  // Trigger next scene
  else
    mp3.play(CMD_LOCAL, 4, false);  // Error sound
}

void setupUserCode()
{
  sceneCallbacks.registerCallback(3, scene3CompletionHandler);
}

Result: Clean, reliable post-scene handling! ✓
```

## Thread Safety Model

```
Thread Context Usage:

┌─────────────────────────────────────────────┐
│ MQTT Task (iot_common_MQTTMessages_T)       │  Main task running WiFi
├─────────────────────────────────────────────┤
│ • Can call registerCallback() ✓             │
│ • Can call queueSceneRun() ✓                │
│ • Cannot safely run callbacks (wrong ctx)   │
│ • Completion happens later in scene task    │
└─────────────────────────────────────────────┘

                    ▼ (via queue)

┌─────────────────────────────────────────────┐
│ Scene Task (iot_common_sceneRunTask_T)      │  Background task
├─────────────────────────────────────────────┤
│ • Executes runScene() ✓                     │
│ • Invokes callbacks directly ✓              │
│ • Only consumer of scene requests ✓         │
│ • No mutex needed (single threaded) ✓       │
└─────────────────────────────────────────────┘

Result: Lock-free, zero-overhead callback system!
```
