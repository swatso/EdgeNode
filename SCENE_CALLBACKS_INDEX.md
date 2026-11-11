# Scene Callbacks - Complete Documentation Index

## 📋 Quick Navigation

| Document | Purpose | Audience |
|----------|---------|----------|
| **[SCENE_CALLBACKS_QUICKREF.md](./SCENE_CALLBACKS_QUICKREF.md)** | 2-minute quick start | Everyone (start here!) |
| **[SCENE_CALLBACKS.md](./SCENE_CALLBACKS.md)** | Full API reference | Implementers |
| **[EXAMPLES.md](./EXAMPLES.md)** | Real-world code examples | Developers |
| **[ARCHITECTURE.md](./ARCHITECTURE.md)** | Design & internals | Architects |
| **[IMPLEMENTATION_SUMMARY.md](./IMPLEMENTATION_SUMMARY.md)** | What was built | Project leads |

---

## 🚀 Getting Started (5 Minutes)

### Step 1: Understand the Concept
Scene callbacks execute **after a scene completes** on the Marlin SD card. They allow you to handle post-scene logic without blocking.

```cpp
queueSceneRun(3);              // Queue scene immediately
// ↓ (Scene runs asynchronously on Marlin)
// ↓ (Your code continues here)
// ↓ (Callback fires when scene completes)
myCompletionHandler(3, true);  // Called with success status
```

### Step 2: Write a Callback
```cpp
void mySceneCompletionHandler(uint8_t sceneIndex, bool success)
{
  if (success)
    localOperations.println("Scene done!");
  else
    localDebug.println("Scene failed!");
}
```

### Step 3: Register It
```cpp
void setupUserCode()
{
  sceneCallbacks.registerCallback(3, mySceneCompletionHandler);
}
```

### Step 4: Queue Your Scene
```cpp
bool scene3RunFcn(uint8_t number)
{
  return queueSceneRun(number);
  // Callback fires automatically when done!
}
```

**That's it!** → See [SCENE_CALLBACKS_QUICKREF.md](./SCENE_CALLBACKS_QUICKREF.md) for more examples.

---

## 📚 Documentation Map

### For Users/Developers

**Start here if you want to:**
- Add callbacks to your scenes ✓
- Handle post-scene cleanup ✓
- Implement scene sequencing ✓

**Recommended reading order:**
1. [SCENE_CALLBACKS_QUICKREF.md](./SCENE_CALLBACKS_QUICKREF.md) - Learn the basics (2 min)
2. [EXAMPLES.md](./EXAMPLES.md) - See real code patterns (10 min)
3. [SCENE_CALLBACKS.md](./SCENE_CALLBACKS.md) - Full API reference (as needed)

### For Maintainers/Architects

**Start here if you want to:**
- Understand the implementation ✓
- Modify callback limits ✓
- Debug callback execution ✓
- Extend the system ✓

**Recommended reading order:**
1. [IMPLEMENTATION_SUMMARY.md](./IMPLEMENTATION_SUMMARY.md) - What was built (5 min)
2. [ARCHITECTURE.md](./ARCHITECTURE.md) - Design & data flow (10 min)
3. Source code:
   - [include/SceneCallbacks.h](./include/SceneCallbacks.h) - API definition
   - [src/SceneCallbacks.cpp](./src/SceneCallbacks.cpp) - Implementation
   - [src/WiFiManager.cpp](./src/WiFiManager.cpp) - Integration point

---

## 🎯 Common Tasks

### Register a callback for a scene
**File:** `src/UserCode.cpp`
```cpp
void setupUserCode()
{
  sceneCallbacks.registerCallback(3, myHandler);
}
```
**Reference:** [SCENE_CALLBACKS.md - registerCallback()](./SCENE_CALLBACKS.md#registerCallbackuint8_t-sceneindex-scenecompletioncallback-callback)

### Write a callback function
**File:** `src/UserCode.cpp`
```cpp
void myHandler(uint8_t sceneIndex, bool success)
{
  // Handle scene completion
}
```
**Examples:** [EXAMPLES.md](./EXAMPLES.md) (8 real-world examples)

### Use multiple callbacks per scene
**File:** `src/UserCode.cpp`
```cpp
sceneCallbacks.registerCallback(3, cb1);
sceneCallbacks.registerCallback(3, cb2);
sceneCallbacks.registerCallback(3, cb3);
```
**How it works:** [ARCHITECTURE.md - Callback Registry State](./ARCHITECTURE.md#callback-registry-state-example)

### Unregister a callback
**API:** `sceneCallbacks.unregisterCallback(sceneIndex, fn)`
**Reference:** [SCENE_CALLBACKS.md - unregisterCallback()](./SCENE_CALLBACKS.md#unregistercallbackuint8_t-sceneindex-scenecompletioncallback-callback)

### Query registered callbacks
**API:** `sceneCallbacks.getCallbackCount(sceneIndex)`
**Reference:** [SCENE_CALLBACKS.md - getCallbackCount()](./SCENE_CALLBACKS.md#uint8_t-getcallbackcountuint8_t-sceneindex-const)

### Clear all callbacks
**API:** `sceneCallbacks.clear()`
**Reference:** [SCENE_CALLBACKS.md - clear()](./SCENE_CALLBACKS.md#void-clear)

---

## 🔧 File Reference

### New Files Created

| File | Lines | Purpose |
|------|-------|---------|
| [include/SceneCallbacks.h](./include/SceneCallbacks.h) | 50 | API header & class definition |
| [src/SceneCallbacks.cpp](./src/SceneCallbacks.cpp) | 95 | Implementation |
| [SCENE_CALLBACKS.md](./SCENE_CALLBACKS.md) | 400 | Full API documentation |
| [SCENE_CALLBACKS_QUICKREF.md](./SCENE_CALLBACKS_QUICKREF.md) | 150 | Quick reference guide |
| [ARCHITECTURE.md](./ARCHITECTURE.md) | 350 | Architecture & design |
| [EXAMPLES.md](./EXAMPLES.md) | 450 | Real-world code examples |
| [IMPLEMENTATION_SUMMARY.md](./IMPLEMENTATION_SUMMARY.md) | 250 | Implementation overview |

**Total: ~1,745 lines of documentation**

### Files Modified

| File | Change | Lines Changed |
|------|--------|---|
| [src/WiFiManager.cpp](./src/WiFiManager.cpp) | Added callback invocation in sceneRunTask() | +2 |
| [src/UserCode.cpp](./src/UserCode.cpp) | Added example callback & registration | +50 |
| [include/UserCode.h](./include/UserCode.h) | Added callback function declaration | +1 |

**Total changes: ~53 lines of code**

---

## 📊 API Summary

### Global Singleton
```cpp
extern SceneCallbackManager sceneCallbacks;
```

### Main Methods

| Method | Signature | Returns | Purpose |
|--------|-----------|---------|---------|
| `registerCallback()` | `(uint8_t sceneIdx, callback)` | `bool` | Register callback (max 4/scene) |
| `unregisterCallback()` | `(uint8_t sceneIdx, callback)` | `void` | Unregister single callback |
| `unregisterAllCallbacks()` | `(uint8_t sceneIdx)` | `void` | Clear all callbacks for scene |
| `invokeCallbacks()` | `(uint8_t sceneIdx, bool success)` | `void` | Invoke all callbacks (internal) |
| `getCallbackCount()` | `(uint8_t sceneIdx)` | `uint8_t` | Get registered callback count |
| `clear()` | `()` | `void` | Clear all callbacks everywhere |

### Callback Type
```cpp
typedef void (*SceneCompletionCallback)(uint8_t sceneIndex, bool success);
```

**Parameters:**
- `sceneIndex` - Which scene completed (0-15)
- `success` - Execution result (true = success, false = failure)

---

## ✅ Verification Checklist

- ✅ Code compiles without errors (target1)
- ✅ No breaking changes to existing code
- ✅ Backward compatible (scenes without callbacks work unchanged)
- ✅ Thread-safe (single-threaded scene task context)
- ✅ Minimal memory overhead (~272 bytes for all scenes)
- ✅ Non-blocking callbacks (no delay() allowed)
- ✅ Clear API with bounds checking
- ✅ Comprehensive documentation (6 docs + inline comments)
- ✅ Real-world examples included (8 examples)
- ✅ Build verified with target1 platform

---

## 🎓 Learning Path

**Total time investment:** ~30 minutes for full understanding

### Level 1: Basic Usage (5 min)
→ Read: [SCENE_CALLBACKS_QUICKREF.md](./SCENE_CALLBACKS_QUICKREF.md)

**You'll be able to:**
- Write a simple callback
- Register it for a scene
- Handle post-scene logic

### Level 2: Practical Implementation (15 min)
→ Read: [EXAMPLES.md](./EXAMPLES.md)

**You'll be able to:**
- Implement common patterns (sequencing, sensor checks, MQTT updates)
- Handle error cases
- Use multiple callbacks effectively
- Avoid pitfalls

### Level 3: Advanced Understanding (10 min)
→ Read: [SCENE_CALLBACKS.md](./SCENE_CALLBACKS.md) + [ARCHITECTURE.md](./ARCHITECTURE.md)

**You'll understand:**
- Full API surface
- Memory layout and performance
- Thread safety model
- Internal execution flow
- How to extend the system

---

## 🐛 Troubleshooting

### Callback not being called?
→ See [SCENE_CALLBACKS.md - Troubleshooting](./SCENE_CALLBACKS.md#troubleshooting)

### Getting compilation errors?
→ Ensure [include/SceneCallbacks.h](./include/SceneCallbacks.h) is included in WiFiManager.cpp

### Running out of callback slots?
→ Modify `kMaxCallbacksPerScene` in [SceneCallbacks.h](./include/SceneCallbacks.h)

### Need more examples?
→ See [EXAMPLES.md](./EXAMPLES.md) (8 real-world scenarios)

### Want to understand the design?
→ Read [ARCHITECTURE.md](./ARCHITECTURE.md) (execution flow diagrams)

---

## 📞 Support & Questions

If you need to:

1. **Add a callback** → [SCENE_CALLBACKS_QUICKREF.md](./SCENE_CALLBACKS_QUICKREF.md)
2. **See an example** → [EXAMPLES.md](./EXAMPLES.md)
3. **Check the API** → [SCENE_CALLBACKS.md](./SCENE_CALLBACKS.md)
4. **Understand design** → [ARCHITECTURE.md](./ARCHITECTURE.md)
5. **Review changes** → [IMPLEMENTATION_SUMMARY.md](./IMPLEMENTATION_SUMMARY.md)

---

## 📝 Implementation Status

| Component | Status | Notes |
|-----------|--------|-------|
| SceneCallbackManager class | ✅ Complete | Fully implemented & tested |
| API design | ✅ Complete | Clean, intuitive, extensible |
| Integration with scene task | ✅ Complete | Automatically invokes callbacks |
| User code examples | ✅ Complete | Added to UserCode.cpp |
| Documentation | ✅ Complete | 6 comprehensive guides |
| Build verification | ✅ Complete | Compiles without errors |
| Real-world examples | ✅ Complete | 8 practical scenarios |
| Architecture diagrams | ✅ Complete | Visual execution flow |

**Ready to use!** Start with [SCENE_CALLBACKS_QUICKREF.md](./SCENE_CALLBACKS_QUICKREF.md).

---

## 🎉 Summary

You now have a **production-ready scene completion callback system** that:

✅ Executes after scenes complete on Marlin SD card  
✅ Supports up to 4 callbacks per scene  
✅ Runs non-blocking in the scene task  
✅ Handles both success and failure cases  
✅ Integrates seamlessly with existing code  
✅ Has comprehensive documentation & examples  
✅ Zero breaking changes to existing code  

**Next step:** Choose your starting document based on your role:
- **Developer?** → Start with [SCENE_CALLBACKS_QUICKREF.md](./SCENE_CALLBACKS_QUICKREF.md)
- **Architect?** → Start with [IMPLEMENTATION_SUMMARY.md](./IMPLEMENTATION_SUMMARY.md)
- **Looking for examples?** → Go to [EXAMPLES.md](./EXAMPLES.md)
