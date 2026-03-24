# LogManager plugin for Unreal Engine 5.6

> ⚠️ This project is released under a non-commercial license.
> Commercial use requires explicit permission.

## ❓ Plugin Overview

**LogManager** is a plugin that enables asynchronous logging during a simulation in Unreal Engine in the form of JSON files. The goal is to provide a tool for researchers who want to log a large amount of data during an experiment without slowing down Unreal’s frame rate.

The plugin can be integrated into a project built for Windows or Android. The provided sample project demonstrates how to use the plugin in an experiment deployed either on a Windows PC or in a Meta Quest 3 VR headset.

The core of the plugin is a library written in C. The library offers a set of generic functions and is intentionally very flexible to allow you to adapt it to the needs of your experiment. 

## 🚀 Getting Started

The LogManagerDemo project is a **demonstration** of how to use LogManager.
Open the Unreal project by clicking on the .uproject file. The system will prompt you to compile the plugins (LogManager and the MetaXR plugin for Meta Quest 3) as well as the project's source code: accept, and the project will launch. 
If you don't have a Meta Quest 3, you can test the project in the Unreal Editor on PC or by packaging the project for Windows (Platforms>Windows>Package Project). 
The project is based on the Unreal 5.6 VR Template and all modifications are in the `Content/LogTestBlueprints/` and `Content/VRTemplate/Blueprint/` directories. See below for details on the changes.

## 🧩 Customization

To adapt the experiment, you must follow several steps:
* Clic on the .uproject file and select **Generate Visual Studio Project Files**
* In the Unreal Editor, double clic on `PLugins/LogManager C++ Classes/LogManager/Public/LogManagerBPFunctionLibrary`
  * It will open the C++ blueprint library that you must edit to adapt the logs to your needs.
  * Use [doxygen documentation](Docs\html\index.html) for a technical documentation.
  * If you are a C++ beginner Follow the [Tutorial: Adapting the plugin](TUTORIAL_AdaptingThePlugin.md) to know how to modify the plugin, add new log events and blueprint nodes.
* In the Unreal Editor, create your own XP, addapt the log interface and the widgets to your need and plug them to the blueprint functions you have created.

## 🎮 LogManagerDemo : Async Logging Example – Unreal Engine 5.6

This project demonstrates how to use the **LogManager plugin** to perform **asynchronous JSON logging** in a virtual reality (VR) experiment built with Unreal Engine 5.6.
The following provides a simple overview of the Unreal project structure. For more details on the Blueprint nodes used to log information and how to customize them (as well as the corresponding logged events), see the [doxygen documentation](Docs\html\index.html) and the [Tutorial: Adapting the plugin](TUTORIAL_AdaptingThePlugin.md).

### Example overview

This example simulates a simple experimental setup where a user navigates inside a **U-shaped corridor** under different conditions. The goal is to showcase how to:

* Log user interactions asynchronously
* Structure experimental data in JSON format
* Combine behavioral data with questionnaire responses

### Important Notes

* This project is a **demonstration**, not a production-ready experiment
* Logging logic is centralized in GameMode for clarity
* The example shows how to combine:
  * Interaction data
  * Questionnaire data

### Experimental Setup

The user can move inside a U-shaped environment. Three experimental conditions are simulated:

* **No Table**
* **Table on the Left**
* **Table on the Right**

```
         No Table:             Table On The Left:        Table On The Right:     
┏━━━━━━━━━━━━━━━━━━━━━━━┓  ┏━━━━━━━━━━━━━━━━━━━━━━━┓  ┏━━━━━━━━━━━━━━━━━━━━━━━┓
┃       ┏━━━━━━━┓       ┃  ┃       ┏━━━━━━━┓       ┃  ┃       ┏━━━━━━━┓       ┃
┃       ┃ CUBES ┃       ┃  ┃       ┃ CUBES ┃       ┃  ┃       ┃ CUBES ┃       ┃
┃       ┗━━━━━━━┛       ┃  ┃       ┗━━━━━━━┛       ┃  ┃       ┗━━━━━━━┛       ┃
┃                       ┃  ┃                       ┃  ┃                       ┃
┃       ┏━━━━━━━┓       ┃  ┃       ┏━━━━━━━┓       ┃  ┃       ┏━━━━━━━┓       ┃
┃       ┃       ┃       ┃  ┃ █████ ┃       ┃       ┃  ┃       ┃       ┃ █████ ┃
┃       ┃       ┃       ┃  ┃       ┃       ┃       ┃  ┃       ┃       ┃       ┃
┣───────┫       ┣───────┫  ┣───────┫       ┣───────┫  ┣───────┫       ┣───────┫
┃   ▲   ┃       ┃   ▼   ┃  ┃   ▲   ┃       ┃   ▼   ┃  ┃   ▲   ┃       ┃   ▼   ┃
┃ START ┃       ┃ARRIVAL┃  ┃ START ┃       ┃ARRIVAL┃  ┃ START ┃       ┃ARRIVAL┃
┗━━━━━━━┛       ┗━━━━━━━┛  ┗━━━━━━━┛       ┗━━━━━━━┛  ┗━━━━━━━┛       ┗━━━━━━━┛
```

At the center of the corridor, the user can interact with **8 cubes** placed on a table.


### User Flow

1. The user creates an **anonymous ID**
2. An experimental condition is assigned
3. A **start questionnaire** is displayed
4. The user performs the task
5. A **final questionnaire** is displayed


### Logged Events

The following events are recorded during an XP run:

* `START_SURVEY` → initial questionnaire (stored separately).
* `XP_START` → experiment start (creation of a new log file)
* `BOX_EVENT` → zone entry/exit
* `CUBE_INTERACTION` → interaction with cubes
* `FINAL_SURVEY` → final questionnaire (stored in current log)
* `XP_STOP` → experiment end (closing of the current log)

In a classic XP, the start survey can be used to collect demographic informations only once per user and the final survey can be used to collect feedback from the user after each run of the XP. 

### Log Files Location

Logs are stored in:

* **Windows**

```
<ProjectName>/Saved/PersistentDownloadDir/LOGS
```

* **Android**

```
/sdcard/Android/data/<PackageName>/files/LOGS
```

### Unreal Project Structure & Modifications

This project is based on the Unreal Engine 5.6 VR Template with several modifications.

#### Logging Interface
* File: `Content/LogTestBlueprints/I_CanLog`
* Provides access to logging events without casting to GameMode.
* Benefits:
  * Cleaner architecture
  * Easy removal of logging logic

#### GameMode (Core Logging Logic)
* File: `Content/VRTemplate/Blueprint/VRGameMode`
* The GameMode centralizes all logging calls.
* **BeginPlay**
  * Register references to scene elements (walls, UI, tables, zones)
  * Detect VR headset: If absent → switch to first-person mode (PC testing)
* **CreateConfigFile**
  * Creates a `config.json` file with User ID and XP Setting (Allows session resume)
* **GetConfigInfos**
  * Reads existing `config.json` file
  * Enables "Continue" option in menu
* **SendStartSurvey**
  * Logs initial questionnaire (`START_SURVEY`) in a separate file `YYYY-MM-DD_HHhMMmSS_idXXXXXXXX_StartSurvey.json`
  * Calls StartXP
* **StartXP**
  * Initializes the map based on XP Setting
  * Creates log file  `YYYY-MM-DD_HHhMMmSS_idXXXXXXXX_setting.json`
  * Adds `XP_START` event
* **CubeInteraction**
  * Logs cube interaction (`CUBE_INTERACTION`)
* **BoxEvent**
  * Logs zone triggers (`BOX_EVENT`)
* **ShowFinalSurvey**
  * Displays final questionnaire
* **SendFinalSurvey**
  * Logs final questionnaire (`FINAL_SURVEY`)
  * Stops experiment
* **EndPlay / StopXP**
  * Logs `XP_STOP`
  * Closes file
  * Resets player position

#### VR Player Implementations
* File: `Content/VRTemplate/Blueprint/VRGameMode`
* Adds:
  * Left/right click interactions (ClickLeft/ClickRight input actions)
  * Reset position function (RestToPlayerStart custom event)
  * Niagara laser pointers for UI interaction (PointerLeftNiagaraSystem/PointerRightNiagaraSystem components)

#### First Person Player (PC fallback)

* File: `BP_FirstPersonPlayer`
* Keyboard movement (AZERTY: ZQSD)
* Mouse interaction (Left Click):
  * Grab objects
  * Interact with UI
* ⚠️ Text input is NOT implemented (use On Screen Widget if needed)

#### Grabbable Cube
* File: `Content/VRTemplate/Blueprint/Grabbable_SmallCube`
* Logs Grab/Drop event with location & timing

#### Box Collision
* File: `Content/VRTemplate/Blueprint/BP_BoxCollision`
* Logs player presence (begin/stop overlapp)

#### Arrival Zone
* File: `Content/LogTestBlueprints/BP_ArrivalZone`
* Triggers final questionnaire

### Table
* File: `Content/LogTestBlueprints/BP_Table`
* Visibility depends on experimental condition

### Wall
* File: `Content/LogTestBlueprints/BP_Wàll`
* Used to isolate user during menus/forms
* Can be shown/hidden dynamically

### UI System

* World Containers used to display widgets in 3D space.
  * File: `Content/LogTestBlueprints/BP_WorldMenu`
  * File: `Content/LogTestBlueprints/BP_WorldStartForm`
  * File: `Content/LogTestBlueprints/BP_WorldFinalForm`

* Widgets
  * File: `Content/LogTestBlueprints/WG_Menu`
    * Create new user
    * Continue experiment
    * Quit application
  * Example questionnaire:
    * Likert scale
    * Binary answers
    * Free text
    * File: `Content/LogTestBlueprints/WG_StartForm`
      * User initial informations
    * File: `Content/LogTestBlueprints/WG_FinalForm`
      * Post-experiment feedback

## 📄 Log File Format (JSON)

Each log file contains a list of events:

```json
[{"event":"EVENT_NAME","gameTime":3.834658,"unixTimeSeconds":1773917869,"unixTimeMilliseconds":139,"frameCount":347665,"data":{...}},
 {"event":"OTHER_EVENT","gameTime":5.840221,"unixTimeSeconds":1773917871,"unixTimeMilliseconds":142,"frameCount":347785,"data":{...}},
 ...]
```

* `event` → event name
* `gameTime` → Unreal time (seconds)
* `unixTimeSeconds` + `unixTimeMilliseconds` → system time
* `frameCount` → frame index
* `data` → custom payload depending on the event

### Supported Data Types

* Integer (`int32`, `int64`, etc.)
* Float
* String
* Boolean
* Structured data (dictionary)

### Examples
config.json (config file)
```json
[{"PlayerId":3055647633038352039,"Setting":"TableRight"}]
```

2026-03-23_17h31m56s_id3055647633038352039_startSurvey.json (Start Survey)
```json
[{"event":"START_SURVEY","gameTime":8.059052,"unixTimeSeconds":1774287116,"unixTimeMilliseconds":177,"frameCount":745,"data":{"Evaluation":3,"Usefullness":true,"Comment":"Great, I just have to move in the scene with teleportation and to manipulate cubes to generate some events in the log. "}}]
```

2026-03-23_17h31m56s_id3055647633038352039_TableRight.json (XP log file)
```json
[{"event":"XP_START","gameTime":8.059052,"unixTimeSeconds":1774287116,"unixTimeMilliseconds":177,"frameCount":745,"data":{"PlayerId":3055647633038352039,"CurrentSetting":"TableRight","StartLocation":{"x":-298.478210,"y":-296.387146,"z":90.212494}}},
{"event":"BOX_EVENT","gameTime":9.942400,"unixTimeSeconds":1774287118,"unixTimeMilliseconds":58,"frameCount":858,"data":{"Number":8,"Location":{"x":600.000000,"y":-380.000000,"z":120.000000},"Inside":true}},
{"event":"BOX_EVENT","gameTime":10.175735,"unixTimeSeconds":1774287118,"unixTimeMilliseconds":291,"frameCount":872,"data":{"Number":8,"Location":{"x":600.000000,"y":-380.000000,"z":120.000000},"Inside":false}},
{"event":"BOX_EVENT","gameTime":11.125744,"unixTimeSeconds":1774287119,"unixTimeMilliseconds":241,"frameCount":929,"data":{"Number":1,"Location":{"x":1300.000000,"y":0.000000,"z":120.000000},"Inside":true}},
{"event":"CUBE_INTERACTION","gameTime":12.409088,"unixTimeSeconds":1774287120,"unixTimeMilliseconds":525,"frameCount":1006,"data":{"IsGrabbed":true,"Timer":0.500005,"Distance":62,"Name":"cube4"}},
{"event":"CUBE_INTERACTION","gameTime":12.475755,"unixTimeSeconds":1774287120,"unixTimeMilliseconds":593,"frameCount":1010,"data":{"IsGrabbed":false,"Timer":0.566672,"Distance":65,"Name":"cube4"}},
{"event":"BOX_EVENT","gameTime":14.675820,"unixTimeSeconds":1774287122,"unixTimeMilliseconds":791,"frameCount":1142,"data":{"Number":1,"Location":{"x":1300.000000,"y":0.000000,"z":120.000000},"Inside":false}},
{"event":"BOX_EVENT","gameTime":14.992490,"unixTimeSeconds":1774287123,"unixTimeMilliseconds":108,"frameCount":1161,"data":{"Number":4,"Location":{"x":970.000000,"y":90.000000,"z":120.000000},"Inside":true}},
{"event":"BOX_EVENT","gameTime":15.225825,"unixTimeSeconds":1774287123,"unixTimeMilliseconds":341,"frameCount":1175,"data":{"Number":4,"Location":{"x":970.000000,"y":90.000000,"z":120.000000},"Inside":false}},
{"event":"FINAL_SURVEY","gameTime":26.309431,"unixTimeSeconds":1774287134,"unixTimeMilliseconds":425,"frameCount":1840,"data":{"Answers":{"How many cubes have you manipulated?":"1","Have you thoroughly explored the area?":"false","Do you know where the generated log file is?":"Yes ! I can find le log file in the Saved\PersistentDownloadDir\LOGS directory !"}}},
{"event":"XP_STOP","gameTime":26.309431,"unixTimeSeconds":1774287134,"unixTimeMilliseconds":425,"frameCount":1840,"data":{}}]
```



## 📄 License

![License: CC BY-NC 4.0](https://img.shields.io/badge/license-CC--BY--NC--4.0-lightgrey)

This project is licensed under the
**Creative Commons Attribution-NonCommercial 4.0 International License (CC BY-NC 4.0)**.

You are free to:

* Use, modify, and share this project for **academic and research purposes**
* Use it for **teaching and non-commercial prototyping**

### ⚠️ Additional Terms for Software Usage

While this project uses a Creative Commons license, it contains **software components (Unreal Engine plugin and native libraries)**. The following additional terms apply:

#### ❌ Commercial Use Prohibited

Commercial use is strictly prohibited, including (but not limited to):

* Use within a company or for-profit organization
* Integration into a commercial product or service
* Internal R&D in a commercial environment
* Use in paid applications, services, or consulting

#### 🔒 Redistribution Restrictions

* You may **not redistribute** this plugin or its binaries (.dll, .so) as part of another project without permission
* You may **not resell or sublicense** this software
* Any public sharing must include proper attribution and a link to this repository

#### 🧩 Third-Party Library

This plugin includes a compiled native library:

* Windows (`.dll`)
* Android (`.so`)

These binaries are provided **without source code** and are covered by the same license terms.
They may not be extracted, reused, or redistributed outside the scope of this project.

#### 🧠 Academic Use

This software is primarily intended for:

* Academic research
* Experimental applications
* Educational purposes

#### 📩 Commercial Licensing

If you are interested in using this software in a commercial context, please contact us.
Commercial licenses can be granted on a case-by-case basis.

#### ⚠️ Disclaimer

This software is provided *"as is"*, without warranty of any kind.

## Planned future developments

* Access to the library source code (.dll, .so): included in the Unreal plugin folder for recompilation with the project. 
* Addition of MacOS and Linux versions of the library to enable packaging for those platforms. 
