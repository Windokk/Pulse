<h1 align="center">Epoch Engine</h1>

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset=".github/EpochLogoDarkMode.png" width="20%">
    <source media="(prefers-color-scheme: light)" srcset=".github/EpochLogoLightMode.png" width="20%">
    <img alt="Fallback image description" src="default-image.png" width="20%">
  </picture>
</p>

<p align="center">
  Yet another game engine<br>
  (full of bugs)
</p>

## Screenshots

## Build Requirements (Windows)

Cmake 3.28.2 or later

C++ 17 Compiler (GCC MinGW recommended)

Qt 6.9.2 
You can verify your qt version using: 
```bash
qtpaths6 --qt-version
```

ZLIB

## How to build editor app :

Run build.bat or build.sh

This will compile CMakeLists everything in root : the engine, the editor app, the editor module, and a basic game module (with the game app)

(As of october 2025, the editor module will try to find Qt6 at : "C:/Qt/6.9.2/mingw_64/lib/cmake/Qt6/" on Windows)


## FEATURES IMPLEMENTED

### Forward Renderer

- PBR shaders/materials (transparency support (transparent & masked))
- Light system : Directionnal, Spot, Point
- Shadow maps : Directionnal CSM, PCF (for all shadow maps)
- Model component (ECS)

#### UI

- Loading fonts
- Text components (ECS)
- UI Pass Type

### Audio System

- 3D Stereo audio
- Audio source component (ECS)

### Filesystem

- Cosem Packer/Unpacker
- Reading/Writing files

### Resources Manager

- Loading/Unloading per folder + file type

### Game Module

- Game module loading at runtime in a DLL
- Code execution across DLL

### Editor Module

- Editor module loaded at runtime in a DLL
- Editor uses Qt for gui

### Debugging

- Debug levels (Log, Info, Warning, Error, Fatal)

### ECS

- Level hierarchy
- Local/Global components ids
- Actor ids

### Events System

- Sending/Recieving events (across custom class/across engine) via a dispatcher

### Inputs System

- Checking for keyboard/mouse inputs
- Cursor state

### Level System

- Loading/Unloading levels
- Adding actors to the level

### Serialization

- Level de-serializer
- Material de-serializer

### Physics System

- Jolt integration
- Physics shapes visualizing

### Time System

- Time queries (delta, fixedDelta, current global (world) time, current app time)
- Timespeed (for physics calculations, particles and custom classes)

### Project system

- Project settings file
- Editor preferences (inside project settings file)



<br>

## ROADMAP FULL ENGINE

- First prototype : December 2025/January 2026
  - All basic feature
  - Example basic project : Pong 3D
- Alpha : Summer 2026
  - Polished Renderer
  - Polished Editor
  - Unit tests
  - Demos (YouTube ?)
- Major First Beta Release 0.1.0 : December 2026/January 2027
  - Bug fixes
  - Documentation (User + Source Code)
  - Optimizations

## ROADMAP FIRST PROTOTYPE 0.1.0 (December 2025)

### Editor (Qt)

- [x] Docking
- [ ] Scene Tree panel
- [ ] Material editor panel
- [ ] Viewport panel
- [ ] Asset Browser
- [ ] Console
- [ ] Stats overlay
  - [ ] Audio infos
  - [ ] Minimal frame profiler (FPS, Speed of each part of the engine...)
  - [ ] Hardware infos (CPU, GPU, RAM) (Name, Vendor, Usage...)

### Rendering

- [ ] Skyboxes + IBL
- [ ] Wireframe rendering
- [ ] Forward+ rendering
- [ ] Ligthmaps
- [ ] FBX importer (multiple meshes)
- [ ] Post processing effects shaders (SSAO, Bloom, Vignette, Tone Mapping, Film Grain, Chromatic Abberration)

#### Serialization

- [ ] Level exporter
- [ ] Material file exporter

#### Debugging

- [ ] Commands (in game/editor)
- [ ] Frame-Debugger (see stats overlay)

#### Animation system

- [ ] Animation data structure importing (from FBX)
- [ ] Animation data manager
- [ ] Skeletal animations
- [ ] Animation Blending
- [ ] Animation State Machine

## Credits

- Models :
  - "Rubik's Cube" (<https://skfb.ly/6U7pp>) by RED2000 is licensed under Creative Commons Attribution (<http://creativecommons.org/licenses/by/4.0/>).
  - "Sponza" Model downloaded from Morgan McGuire's [Computer Graphics Archive](https://casual-effects.com/data)
