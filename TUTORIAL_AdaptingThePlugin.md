# Adapting the LogManager Plugin to Your Experiment
### A Step-by-Step Guide for Developers New to C++

> **Who is this guide for?**
> You are comfortable with Unreal Engine 5 and Blueprints, but you have little or no experience with C++.
> This guide walks you through every file you need to modify to adapt the LogManager plugin to a new experiment, using a real research study as a concrete example throughout.

---

## Table of Contents

1. [Overview: How the Plugin Works](#1-overview-how-the-plugin-works)
2. [The Concrete Example: A VR Pedestrian Study](#2-the-concrete-example-a-vr-pedestrian-study)
3. [Step 1 — Identify the Events You Need to Log](#step-1--identify-the-events-you-need-to-log)
4. [Step 2 — Define Your Events in `LogManagerConstants.h`](#step-2--define-your-events-in-logmanagerconstantsh)
5. [Step 3 — Define Your Experimental Conditions in `LogManagerConstants.h`](#step-3--define-your-experimental-conditions-in-logmanagerconstantsh)
6. [Step 4 — Declare Your Blueprint Functions in `LogManagerBPFunctionLibrary.h`](#step-4--declare-your-blueprint-functions-in-logmanagerbpfunctionlibraryh)
7. [Step 5 — Implement Your Blueprint Functions in `LogManagerBPFunctionLibrary.cpp`](#step-5--implement-your-blueprint-functions-in-logmanagerbpfunctionlibrarycpp)
8. [Step 6 — Call Your Functions from Blueprints](#step-6--call-your-functions-from-blueprints)
9. [Understanding the JSON Output](#understanding-the-json-output)
10. [Common Mistakes and How to Avoid Them](#common-mistakes-and-how-to-avoid-them)
11. [Quick Reference: C++ Types vs Blueprint Types](#quick-reference-c-types-vs-blueprint-types)

---

## 1. Overview: How the Plugin Works

Before touching any file, it helps to understand the overall architecture with three layers:

```
Your Blueprint  ──calls──►  LogManagerBPFunctionLibrary  ──calls──►  FLogManagerModule  ──calls──►  liblogmanager.dll
(UE5 visual)                (the file you modify)                    (do not modify)                (do not modify)
```

- **`liblogmanager.dll`** — a compiled C library that does the actual file writing. You never touch this.
- **`FLogManagerModule`** (`LogManager.h` / `LogManager.cpp`) — the UE5 module that loads the DLL and exposes its functions as C++ methods. You never touch this either.
- **`LogManagerBPFunctionLibrary`** — this is your workspace. You add one C++ function here for each event you want to log.
- **`LogManagerConstants.h`** — you define your event names and experimental conditions here.

**The files you will modify:**
| File | What you change |
|---|---|
| `LogManagerConstants.h` | Add your event names (`EEvent`) and conditions (`ESetting`) |
| `LogManagerBPFunctionLibrary.h` | Declare a function for each event |
| `LogManagerBPFunctionLibrary.cpp` | Implement each function (what data gets written) |

---

## 2. The Concrete Example: A VR Pedestrian Study

Throughout this guide, we use a real experiment as our working example: a study investigating how footstep sounds influence collision-avoidance behavior between a real participant and a virtual agent in VR ([Hufschmitt et al., IEEE VR 2026](https://inria.hal.science/hal-05514579v1)).

**The experiment in a nutshell:**
- A participant walks in a 15m × 15m virtual room toward a target (a fox statue).
- A virtual agent (VA) crosses their path from the left or right.
- Two conditions are compared: with footstep sounds (FS) vs. without (NFS).
- The study records locomotion, gaze, and head tracking at 20 Hz throughout each trial.

**The events that need to be logged:**

| Event name | When it fires | Key data recorded |
|---|---|---|
| `XP_START` | Start of a trial | Participant ID, condition (FS/NFS), agent side (left/right), volumes, walking speed |
| `PLAYER_INFOS` | Every simulation step (~20 Hz) | Head position/rotation/velocity, hand positions, 2D walking direction |
| `NPC_INFOS` | Every simulation step (~20 Hz) | Agent position, velocity, sound volume, occlusion state |
| `EYE_TRACKING` | Every simulation step (~20 Hz) | Gaze origin, gaze direction, focused object |
| `XP_END` | End of a trial | *(no extra data — just marks the end)* |

---

## Step 1 — Identify the Events You Need to Log

Before writing a single line of code, answer these questions on paper:

**1. What are the distinct moments in your experiment that need to be recorded?**

Think of each *type* of thing that happens, not individual instances. In the pedestrian study:
- The trial starts → `XP_START`
- Every frame, the participant's body moves → `PLAYER_INFOS`
- Every frame, the agent moves → `NPC_INFOS`
- Every frame, the participant looks somewhere → `EYE_TRACKING`
- The trial ends → `XP_END`

**2. For each event, what data do you need?**

Write it down as a simple list. For `PLAYER_INFOS`:
- 2D walking direction (x, y)
- Head position (x, y, z)
- Head rotation (x, y, z)
- Head velocity (x, y, z)
- Left hand position (x, y, z)
- Left hand rotation (x, y, z)
- Right hand position (x, y, z)
- Right hand rotation (x, y, z)

**3. What are your experimental conditions?**

In the pedestrian study: `FootstepSound` (FS) vs `NoFootstepSound` (NFS). These go into `ESetting`.

> **Tip:** Sketch the JSON structure you want on paper before coding. It is much easier to adjust a sketch than to rewrite C++ code.

---

## Step 2 — Define Your Events in `LogManagerConstants.h`

Open `LogManagerConstants.h`. Find the `EEvent` enum and **replace** the existing values with yours.

**⚠️ Important rules:**
- Do **not** rename the enum itself (`EEvent`). Only change the values inside it.
- Each value becomes the `"event"` field name in your JSON log files.
- Keep the values in the logical order they occur in your experiment.

**Before (original plugin):**
```cpp
UENUM(BlueprintType)
enum class EEvent : uint8 {
    START_SURVEY,
    XP_START,
    BOX_EVENT,
    CUBE_INTERACTION,
    XP_STOP,
    FINAL_SURVEY
};
```

**After (adapted for the pedestrian study):**
```cpp
UENUM(BlueprintType)
enum class EEvent : uint8 {
    // Marks the beginning of a trial: records participant ID, condition, VA side, and calibration data
    XP_START,
    // Records the participant's head, hand positions and walking direction at each simulation step
    PLAYER_INFOS,
    // Records the virtual agent's position, velocity, sound level, and occlusion state at each simulation step
    NPC_INFOS,
    // Records gaze origin, gaze direction, and focused object at each simulation step
    EYE_TRACKING,
    // Marks the end of a trial; its presence confirms the trial completed normally
    XP_END
};
```

> **Why add comments?** The `EEvent` enum is the central reference for everyone working on the project. Clear comments make it immediately obvious what each event represents without having to open other files.

**How to change the directory used to store the log files ?**

The logs directory is defined by the `GetBaseLogDirectory()` function in `LogManagerConstants.h`. We recommend creating your log directory inside the `Project Persistent Download Dir` folder, which will be present and accessible on both Windows and Android. If you want, for example, to renamme the directory "PedestrianLogs" : 

```cpp
	inline FString GetBaseLogDirectory() {
		// log files directory
		return FPaths::Combine(FPaths::ProjectPersistentDownloadDir(), "PedestrianLogs/");
	}
```
---

## Step 3 — Define Your Experimental Conditions in `LogManagerConstants.h`

Find the `ESetting` enum in the same file. **Replace** its values with your own experimental conditions.

**⚠️ Important rules:**
- Do **not** rename the enum itself (`ESetting`). Only change the values inside it.
- Always keep the `Count` sentinel at the end — it is used internally to cycle through conditions automatically.
- The `DisplayName` in `UMETA` is what appears in the Blueprint editor dropdown.

**Before (original plugin):**
```cpp
UENUM(BlueprintType)
enum class ESetting : uint8 {
    TableLeft    UMETA(DisplayName = "Table On The Left"),
    NoTable      UMETA(DisplayName = "No Table"),
    TableRight   UMETA(DisplayName = "Table On The Right"),
    Count        UMETA(Hidden)
};
```

**After (adapted for the pedestrian study):**
```cpp
UENUM(BlueprintType)
enum class ESetting : uint8 {
    FootstepSound      UMETA(DisplayName = "Footstep Sound"),      // FS condition: VA footsteps are audible
    NoFootstepSound    UMETA(DisplayName = "No Footstep Sound"),   // NFS condition: VA footsteps are silent
    Count              UMETA(Hidden)
};
```

---

## Step 4 — Declare Your Blueprint Functions in `LogManagerBPFunctionLibrary.h`

Open `LogManagerBPFunctionLibrary.h`. This file is the *menu* of functions available in Blueprints.

For each event you defined in Step 2, add one `UFUNCTION` declaration. The pattern is always the same:

```cpp
UFUNCTION(BlueprintCallable, meta = (DisplayName = "Your Display Name"), Category = "LOGLibrary")
static void WriteLog_YourEventName(float gameTime, /* your parameters here */);
```

**Rules for parameters:**
- Always start with `float gameTime` — this is the in-game timestamp, required by every log entry.
- Use `FVector` for 3D positions, rotations, and velocities.
- Use `FVector2D` for 2D directions.
- Use `float` for scalar values (volumes, speeds).
- Use `bool` for binary states (is occluded, is visible).
- Use `FString` for text labels.
- Use `int64` for participant IDs (large numbers).
- Use `int32` for counters and step indices.

**Complete declaration for the pedestrian study** — add these inside the `UCLASS` block, replacing the original functions:

```cpp
/**
 * Marks the start of a trial. Opens a new log file and records the initial
 * experimental parameters: participant ID, condition, agent side, and audio calibration data.
 */
UFUNCTION(BlueprintCallable, meta = (DisplayName = "XP Start"), Category = "LOGLibrary")
static void WriteLog_XPStart(float gameTime, int64 userId, ESetting currentSetting,
                              FString npcSide, float ambientVolume,
                              float minVolume, float maxVolume,
                              float playerMedianWalkSpeed, float umansTimeStep);

/**
 * Records the participant's body state at one simulation step.
 * Intended to be called every frame or at a fixed frequency (e.g. 20 Hz).
 */
UFUNCTION(BlueprintCallable, meta = (DisplayName = "Player Infos"), Category = "LOGLibrary")
static void WriteLog_PlayerInfos(float gameTime, float umansStartTime, int32 simulationStep,
                                  FVector2D direction2D,
                                  FVector headLocation, FVector headRotation, FVector headVelocity,
                                  FVector handLLocation, FVector handLRotation,
                                  FVector handRLocation, FVector handRRotation);

/**
 * Records the virtual agent's state at one simulation step.
 * Intended to be called every frame or at a fixed frequency (e.g. 20 Hz).
 */
UFUNCTION(BlueprintCallable, meta = (DisplayName = "NPC Infos"), Category = "LOGLibrary")
static void WriteLog_NpcInfos(float gameTime, float umansStartTime, int32 simulationStep,
                               FVector2D direction2D,
                               FVector location3D, FVector rotation3D, FVector velocity3D,
                               float soundVolume, bool isOccluded,
                               FString occlusionType, bool isVisibleByPlayer);

/**
 * Records the participant's eye-tracking data at one simulation step.
 * Intended to be called every frame or at a fixed frequency (e.g. 20 Hz).
 */
UFUNCTION(BlueprintCallable, meta = (DisplayName = "Eye Tracking"), Category = "LOGLibrary")
static void WriteLog_EyeTracking(float gameTime, float umansStartTime, int32 simulationStep,
                                  FVector gazeOrigin, FVector gazeDirection,
                                  FString focusObject, FString focusObjectSide);

/**
 * Marks the end of a trial and closes the log file.
 * The presence of this event in the file confirms the trial ended normally.
 */
UFUNCTION(BlueprintCallable, meta = (DisplayName = "XP End"), Category = "LOGLibrary")
static void WriteLog_XPEnd(float gameTime);
```

> **What is `UFUNCTION(BlueprintCallable, ...)`?**
> This is the Unreal Engine macro that makes your C++ function visible and callable from a Blueprint graph. Without it, Blueprint cannot see the function at all. `DisplayName` controls the label shown on the Blueprint node. `Category` controls which section of the Blueprint search menu it appears in.

---

## Step 5 — Implement Your Blueprint Functions in `LogManagerBPFunctionLibrary.cpp`

Open `LogManagerBPFunctionLibrary.cpp`. This is where you specify exactly what data gets written for each event.

Every function follows the same pattern — here is the template to understand before writing your own:

```cpp
void ULogManagerBPFunctionLibrary::WriteLog_YourEvent(float gameTime, /* parameters */)
{
    // nbFields = the number of top-level key-value pairs you will write
    if (FLogManagerModule::Get().BeginLogEntry(EEvent::YOUR_EVENT, gameTime, nbFields)) {

        // --- Write your data fields here ---
        FLogManagerModule::Get().AddFloatData("MyFloat", myFloatValue);
        FLogManagerModule::Get().AddBoolData("MyBool", myBoolValue);
        FLogManagerModule::Get().AddStringData("MyString", myStringValue);

        // For a nested object (e.g. a 3D vector), use AddComposedData:
        FLogManagerModule::Get().AddComposedData("MyVector", 3); // 3 sub-fields follow
            FLogManagerModule::Get().AddFloatData("x", myVector.X);
            FLogManagerModule::Get().AddFloatData("y", myVector.Y);
            FLogManagerModule::Get().AddFloatData("z", myVector.Z);

        // Always end with this — never forget it!
        FLogManagerModule::Get().FinalizeConfigOrLogEntry();
    }
}
```

**Key rules:**
- `nbFields` must match exactly the number of `Add*Data` and `AddComposedData` calls at the top level. A nested object (`AddComposedData`) counts as **one** top-level field.
- Always call `FinalizeConfigOrLogEntry()` at the end, even if something went wrong. It releases an internal lock.
- The indentation under `AddComposedData` is purely visual — it helps you count sub-fields but has no effect on the code.

---

### The file lifecycle: open in `XP_START`, close in `XP_END`

A critical concept to understand before writing any implementation: **the plugin writes all events for a trial into a single JSON file**. That file must be explicitly opened at the start of the trial and explicitly closed at the end. This is the responsibility of `WriteLog_XPStart` and `WriteLog_XPEnd` respectively — no other function opens or closes a file.

The diagram below shows the full lifecycle of a log file across one trial:

```
WriteLog_XPStart()          WriteLog_PlayerInfos()       WriteLog_XPEnd()
       │                    WriteLog_NpcInfos()                  │
       │                    WriteLog_EyeTracking()               │
       ▼                           ▼                             ▼
 ┌──────────────┐          ┌───────────────────┐         ┌─────────────────┐
 │ OpenNewFile  │          │  BeginLogEntry()  │         │ BeginLogEntry() │
 │ BeginEntry   │──────────│  Add*Data() ...   │── ... ──│ Finalize()      │
 │ Add*Data()   │          │  Finalize()       │         │ CloseFile()     │
 │ Finalize()   │          └───────────────────┘         └─────────────────┘
 └──────────────┘
  File is open here                                        File is closed here
```

**What happens in `WriteLog_XPStart` — opening the file:**

```cpp
void ULogManagerBPFunctionLibrary::WriteLog_XPStart(...)
{
    // ① Get the current date and time from the system clock
    FDateTime date = FDateTime::Now();

    // ② Build the full file path by concatenating strings.
    //    LogManagerConstants::GetBaseLogDirectory() returns the LOGS/ folder path.
    //    The file name encodes the timestamp, participant ID, and condition,
    //    so that each trial produces a uniquely named file that cannot be overwritten.
    //    Example result: .../LOGS/2025-06-17_14h32m05s_id1_FootstepSound.json
    FString filepath = LogManagerConstants::GetBaseLogDirectory() +
        FString::FromInt(date.GetYear()) + "-" +
        FString::Printf(TEXT("%02d"), date.GetMonth()) + "-" +   // %02d = always 2 digits, e.g. "06"
        FString::Printf(TEXT("%02d"), date.GetDay()) + "_" +
        FString::Printf(TEXT("%02d"), date.GetHour()) + "h" +
        FString::Printf(TEXT("%02d"), date.GetMinute()) + "m" +
        FString::Printf(TEXT("%02d"), date.GetSecond()) + "s_id" +
        LexToString((uint64)userId) + "_" +                      // LexToString converts a number to text
        settingToString(currentSetting) + ".json";               // settingToString converts ESetting to text

    // ③ Actually create and open the file on disk.
    //    If this fails (e.g. the LOGS/ directory does not exist or is read-only),
    //    the function stops here — return prevents writing to a null file handle.
    if (!FLogManagerModule::Get().OpenNewJSonFile(filepath)) return;

    // ④ The file is now open. Write the XP_START event as the first entry.
    //    All subsequent WriteLog_* calls during this trial will append to this same file.
    if (FLogManagerModule::Get().BeginLogEntry(EEvent::XP_START, gameTime, 7)) {
        // ... Add*Data calls ...
        FLogManagerModule::Get().FinalizeConfigOrLogEntry();
    }
}
```

> **Why encode the timestamp in the file name?**
> Each call to `WriteLog_XPStart` creates a brand new file. If two trials used the same file name, the second would silently overwrite the first. The timestamp + participant ID + condition combination makes every file name unique, even if a participant repeats a trial within the same second.

> **What does `TEXT("%02d")` mean?**
> `%02d` is a formatting instruction: print the integer as a decimal number, using at least 2 digits, padding with a leading zero if needed. This ensures that months, days, hours, minutes and seconds are always 2 characters wide (e.g. `"06"` instead of `"6"`), which keeps file names consistently sortable in a file explorer.

**What happens in `WriteLog_XPEnd` — closing the file:**

```cpp
void ULogManagerBPFunctionLibrary::WriteLog_XPEnd(float gameTime)
{
    // ① Write the XP_END event with no data fields (nbFields = 0).
    //    Its sole purpose is to act as a completion marker:
    //    if the file ends with XP_END, the trial finished normally.
    //    If the file ends with another event, the trial was interrupted.
    if (FLogManagerModule::Get().BeginLogEntry(EEvent::XP_END, gameTime, 0)) {
        FLogManagerModule::Get().FinalizeConfigOrLogEntry();
    }

    // ② Flush all buffered data to disk and close the file.
    //    After this call, the file is complete and no more entries can be added.
    //    The next trial will open a new file when WriteLog_XPStart is called again.
    FLogManagerModule::Get().FlushAndCloseJSonFile();
}
```

> **Why call `FlushAndCloseJSonFile()` outside the `if` block?**
> The `if (BeginLogEntry(...))` guard only controls whether the event is written. The file must always be closed at the end of a trial, regardless of whether the final event write succeeded. Placing `FlushAndCloseJSonFile()` outside the `if` guarantees the file is never left open, even if `BeginLogEntry` failed.

> **What does "flush" mean?**
> For performance reasons, operating systems often keep file writes in a memory buffer rather than writing them to disk immediately. "Flushing" forces all buffered data to be written to disk right now. Without an explicit flush, a crash or power loss at the end of a trial could result in a partially written or empty file on disk.

---

### Complete implementation for the pedestrian study

```cpp
void ULogManagerBPFunctionLibrary::WriteLog_XPStart(
    float gameTime, int64 userId, ESetting currentSetting,
    FString npcSide, float ambientVolume,
    float minVolume, float maxVolume,
    float playerMedianWalkSpeed, float umansTimeStep)
{
    FDateTime date = FDateTime::Now();
    // Build a timestamped file name: YYYY-MM-DD_HHhMMmSSs_id<userId>_<setting>.json
    FString filepath = LogManagerConstants::GetBaseLogDirectory() +
        FString::FromInt(date.GetYear()) + "-" +
        FString::Printf(TEXT("%02d"), date.GetMonth()) + "-" +
        FString::Printf(TEXT("%02d"), date.GetDay()) + "_" +
        FString::Printf(TEXT("%02d"), date.GetHour()) + "h" +
        FString::Printf(TEXT("%02d"), date.GetMinute()) + "m" +
        FString::Printf(TEXT("%02d"), date.GetSecond()) + "s_id" +
        LexToString((uint64)userId) + "_" + settingToString(currentSetting) + ".json";

    if (!FLogManagerModule::Get().OpenNewJSonFile(filepath)) return;
    UE_LOG(LogManager, Log, TEXT("Create file %s\n"), *filepath);

    // 7 top-level fields: PlayerId, NpcSide, AmbientVolume, MinVolume,
    //                     MaxVolume, PlayerMedianWalkSpeed, UmansTimeStep
    if (FLogManagerModule::Get().BeginLogEntry(EEvent::XP_START, gameTime, 7)) {
        FLogManagerModule::Get().AddULongLongData("PlayerId", (uint64)userId);
        FLogManagerModule::Get().AddStringData("NpcSide", npcSide);
        FLogManagerModule::Get().AddFloatData("AmbientVolume", ambientVolume);
        FLogManagerModule::Get().AddFloatData("MinVolume", minVolume);
        FLogManagerModule::Get().AddFloatData("MaxVolume", maxVolume);
        FLogManagerModule::Get().AddFloatData("PlayerMedianWalkSpeed", playerMedianWalkSpeed);
        FLogManagerModule::Get().AddFloatData("UmansTimeStep", umansTimeStep);
        FLogManagerModule::Get().FinalizeConfigOrLogEntry();
    }
}

void ULogManagerBPFunctionLibrary::WriteLog_PlayerInfos(
    float gameTime, float umansStartTime, int32 simulationStep,
    FVector2D direction2D,
    FVector headLocation, FVector headRotation, FVector headVelocity,
    FVector handLLocation, FVector handLRotation,
    FVector handRLocation, FVector handRRotation)
{
    // 9 top-level fields: UmansStartTime, SimulationStep, Direction2D,
    //                     Head_location, Head_rotation, Head_velocity,
    //                     HandL_location, HandL_rotation,
    //                     HandR_location, HandR_rotation
    if (FLogManagerModule::Get().BeginLogEntry(EEvent::PLAYER_INFOS, gameTime, 10)) {
        FLogManagerModule::Get().AddFloatData("UmansStartTime", umansStartTime);
        FLogManagerModule::Get().AddIntData("SimulationStep", simulationStep);

        FLogManagerModule::Get().AddComposedData("Direction2D", 2);
            FLogManagerModule::Get().AddFloatData("x", direction2D.X);
            FLogManagerModule::Get().AddFloatData("y", direction2D.Y);

        FLogManagerModule::Get().AddComposedData("Head_location", 3);
            FLogManagerModule::Get().AddFloatData("x", headLocation.X);
            FLogManagerModule::Get().AddFloatData("y", headLocation.Y);
            FLogManagerModule::Get().AddFloatData("z", headLocation.Z);

        FLogManagerModule::Get().AddComposedData("Head_rotation", 3);
            FLogManagerModule::Get().AddFloatData("x", headRotation.X);
            FLogManagerModule::Get().AddFloatData("y", headRotation.Y);
            FLogManagerModule::Get().AddFloatData("z", headRotation.Z);

        FLogManagerModule::Get().AddComposedData("Head_velocity", 3);
            FLogManagerModule::Get().AddFloatData("x", headVelocity.X);
            FLogManagerModule::Get().AddFloatData("y", headVelocity.Y);
            FLogManagerModule::Get().AddFloatData("z", headVelocity.Z);

        FLogManagerModule::Get().AddComposedData("HandL_location", 3);
            FLogManagerModule::Get().AddFloatData("x", handLLocation.X);
            FLogManagerModule::Get().AddFloatData("y", handLLocation.Y);
            FLogManagerModule::Get().AddFloatData("z", handLLocation.Z);

        FLogManagerModule::Get().AddComposedData("HandL_rotation", 3);
            FLogManagerModule::Get().AddFloatData("x", handLRotation.X);
            FLogManagerModule::Get().AddFloatData("y", handLRotation.Y);
            FLogManagerModule::Get().AddFloatData("z", handLRotation.Z);

        FLogManagerModule::Get().AddComposedData("HandR_location", 3);
            FLogManagerModule::Get().AddFloatData("x", handRLocation.X);
            FLogManagerModule::Get().AddFloatData("y", handRLocation.Y);
            FLogManagerModule::Get().AddFloatData("z", handRLocation.Z);

        FLogManagerModule::Get().AddComposedData("HandR_rotation", 3);
            FLogManagerModule::Get().AddFloatData("x", handRRotation.X);
            FLogManagerModule::Get().AddFloatData("y", handRRotation.Y);
            FLogManagerModule::Get().AddFloatData("z", handRRotation.Z);

        FLogManagerModule::Get().FinalizeConfigOrLogEntry();
    }
}

void ULogManagerBPFunctionLibrary::WriteLog_NpcInfos(
    float gameTime, float umansStartTime, int32 simulationStep,
    FVector2D direction2D,
    FVector location3D, FVector rotation3D, FVector velocity3D,
    float soundVolume, bool isOccluded,
    FString occlusionType, bool isVisibleByPlayer)
{
    // 10 top-level fields
    if (FLogManagerModule::Get().BeginLogEntry(EEvent::NPC_INFOS, gameTime, 10)) {
        FLogManagerModule::Get().AddFloatData("UmansStartTime", umansStartTime);
        FLogManagerModule::Get().AddIntData("SimulationStep", simulationStep);

        FLogManagerModule::Get().AddComposedData("Direction2D", 2);
            FLogManagerModule::Get().AddFloatData("x", direction2D.X);
            FLogManagerModule::Get().AddFloatData("y", direction2D.Y);

        FLogManagerModule::Get().AddComposedData("Location3D", 3);
            FLogManagerModule::Get().AddFloatData("x", location3D.X);
            FLogManagerModule::Get().AddFloatData("y", location3D.Y);
            FLogManagerModule::Get().AddFloatData("z", location3D.Z);

        FLogManagerModule::Get().AddComposedData("Rotation3D", 3);
            FLogManagerModule::Get().AddFloatData("x", rotation3D.X);
            FLogManagerModule::Get().AddFloatData("y", rotation3D.Y);
            FLogManagerModule::Get().AddFloatData("z", rotation3D.Z);

        FLogManagerModule::Get().AddComposedData("Velocity3D", 3);
            FLogManagerModule::Get().AddFloatData("x", velocity3D.X);
            FLogManagerModule::Get().AddFloatData("y", velocity3D.Y);
            FLogManagerModule::Get().AddFloatData("z", velocity3D.Z);

        FLogManagerModule::Get().AddFloatData("Sound_volume", soundVolume);
        FLogManagerModule::Get().AddBoolData("IsOccluded", isOccluded);
        FLogManagerModule::Get().AddStringData("OcclusionType", occlusionType);
        FLogManagerModule::Get().AddBoolData("IsVisibleByPlayer", isVisibleByPlayer);

        FLogManagerModule::Get().FinalizeConfigOrLogEntry();
    }
}

void ULogManagerBPFunctionLibrary::WriteLog_EyeTracking(
    float gameTime, float umansStartTime, int32 simulationStep,
    FVector gazeOrigin, FVector gazeDirection,
    FString focusObject, FString focusObjectSide)
{
    // 6 top-level fields
    if (FLogManagerModule::Get().BeginLogEntry(EEvent::EYE_TRACKING, gameTime, 6)) {
        FLogManagerModule::Get().AddFloatData("UmansStartTime", umansStartTime);
        FLogManagerModule::Get().AddIntData("SimulationStep", simulationStep);

        FLogManagerModule::Get().AddComposedData("GazeOrigin", 3);
            FLogManagerModule::Get().AddFloatData("x", gazeOrigin.X);
            FLogManagerModule::Get().AddFloatData("y", gazeOrigin.Y);
            FLogManagerModule::Get().AddFloatData("z", gazeOrigin.Z);

        FLogManagerModule::Get().AddComposedData("GazeDirection", 3);
            FLogManagerModule::Get().AddFloatData("x", gazeDirection.X);
            FLogManagerModule::Get().AddFloatData("y", gazeDirection.Y);
            FLogManagerModule::Get().AddFloatData("z", gazeDirection.Z);

        FLogManagerModule::Get().AddStringData("FocusObject", focusObject);
        FLogManagerModule::Get().AddStringData("FocusObjectSide", focusObjectSide);

        FLogManagerModule::Get().FinalizeConfigOrLogEntry();
    }
}

void ULogManagerBPFunctionLibrary::WriteLog_XPEnd(float gameTime)
{
    // Write the XP_END marker event (no data fields)
    if (FLogManagerModule::Get().BeginLogEntry(EEvent::XP_END, gameTime, 0)) {
        FLogManagerModule::Get().FinalizeConfigOrLogEntry();
    }
    // Flush all buffered data to disk and close the file — see explanation above
    FLogManagerModule::Get().FlushAndCloseJSonFile();
}
```

---

## Step 6 — Call Your Functions from Blueprints

Once you have compiled the plugin (Build > Build in Visual Studio, or Compile in the UE5 editor), your new functions appear in any Blueprint graph under the **LOGLibrary** category.

**Typical Blueprint wiring for the pedestrian study:**

```
BeginPlay  ──►  Get Config File Infos  ──►  XP Start  (call once per trial)
                                              │
                        ┌─────────────────────┘
                        ▼
               Event Tick (or UMANS callback)  ──►  Player Infos
                                               ──►  NPC Infos
                                               ──►  Eye Tracking
                        │
                        ▼
               Trial End trigger  ──►  XP End  (call once per trial)
                                   ──►  Create Config File  (to prepare next participant)
```

> **Important:** `WriteLog_PlayerInfos`, `WriteLog_NpcInfos`, and `WriteLog_EyeTracking` are called at high frequency (every simulation step). Make sure your Blueprint calls them inside a rate-limited tick or a dedicated timer, not on the main `Event Tick` which runs at the full frame rate and would generate extremely large log files.

---

## Understanding the JSON Output

Each call to `WriteLog_*` appends one object to the JSON array in the current log file. Here is what the output looks like for the pedestrian study, matching the format you already have:

```json
[
  {
    "event": "XP_START",
    "gameTime": 496.275024,
    "unixTimeSeconds": 1751897653,
    "unixTimeMilliseconds": 507,
    "frameCount": 157138,
    "datas": {
      "PlayerId": 1,
      "NpcSide": "RIGHT",
      "AmbientVolume": 50.0,
      "MinVolume": 19.18,
      "MaxVolume": 65.19,
      "PlayerMedianWalkSpeed": 96.29,
      "UmansTimeStep": 0.05
    }
  },
  {
    "event": "EYE_TRACKING",
    "gameTime": 499.597900,
    "datas": {
      "UmansStartTime": 499.531494,
      "SimulationStep": 0,
      "GazeOrigin": { "x": 168.58, "y": 167.91, "z": 149.20 },
      "GazeDirection": { "x": 0.697, "y": 0.706, "z": -0.118 },
      "FocusObject": "Floor",
      "FocusObjectSide": "RIGHT"
    }
  },
  {
    "event": "XP_END",
    "gameTime": 521.981995,
    "datas": {}
  }
]
```

The fields `event`, `gameTime`, `unixTimeSeconds`, `unixTimeMilliseconds`, and `frameCount` are written automatically by `BeginLogEntry()`. You only write what goes inside `"datas"`.

---

## Common Mistakes and How to Avoid Them

| Mistake | Symptom | Fix |
|---|---|---|
| `nbFields` does not match the number of `Add*Data` calls | Corrupted or truncated JSON | Count carefully: each `AddComposedData` is 1 field, regardless of how many sub-fields follow |
| Forgetting `FinalizeConfigOrLogEntry()` | All subsequent writes are silently blocked | Always call it, even inside an `if` branch that returns early |
| Calling `WriteLog_XPEnd` without a prior `WriteLog_XPStart` | Crash or write to a null file handle | Always open a file before writing to it |
| Renaming `EEvent` or `ESetting` | Compilation errors across the whole plugin | Only change the *values* inside the enum, never the enum name or type |
| Using `//` comments in the `.h` instead of `/** */` | Comments invisible in generated Doxygen documentation | Use `/** ... */` for all documentation comments |
| Missing `Count` sentinel in `ESetting` | `CreateNewConfigFile` cycles incorrectly | Always keep `Count UMETA(Hidden)` as the last value of `ESetting` |

---

## Quick Reference: C++ Types vs Blueprint Types

| C++ type | Blueprint type | Use it for |
|---|---|---|
| `float` | Float | Positions, angles, velocities, volumes, timestamps |
| `int32` | Integer | Counters, step indices, small IDs |
| `int64` | Integer64 | Participant IDs, large counters |
| `bool` | Boolean | Binary states (is grabbed, is occluded, is visible) |
| `FString` | String | Text labels, enum names, free-text comments |
| `FVector` | Vector | 3D position, rotation, velocity |
| `FVector2D` | Vector2D | 2D direction, ground-plane position |
| `FName` | Name | Object identifiers (read-only labels) |
| `TMap<FString, FString>` | Map (String→String) | Survey question/answer pairs |

---

*This tutorial was written for the LogManager Plugin — Copyright © 2025 AIPIC Team, Research Center of Saint-Cyr Coetquidan. Licensed under CC BY-NC 4.0.*
