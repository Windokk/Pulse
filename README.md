<h1 align="center">Pulse Engine</h1>

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset=".github/PulseLogoDarkMode.png" width="15%">
    <source media="(prefers-color-scheme: light)" srcset=".github/PulseLogoLightMode.png" width="15%">
    <img alt="Fallback image description" src="default-image.png" width="20%">
  </picture>
</p>

<p align="center">
  Yet another game engine<br>
  (full of bugs)
</p>

## Screenshots



## Build Requirements (Windows)

- CMake 3.28.2 or later
- C++ 17 Compiler (GCC MinGW recommended)
- Qt 6.9.2
- GLFW
- ZLIB
- FMOD API
- QtAdvancedDocking

## Build Requirements (Linux) :

### WIP

## How to build :

Run build.bat or build.sh (depending on your OS)

This will compile everything from root : the engine, the editor app, the editor module, and the game module (loaded with the game app)

(As of october 2025, the editor module will try to find Qt6 at : "C:/Qt/6.9.2/mingw_64/lib/cmake/Qt6/" on Windows, you can chnage it in the parent CMakeLists.txt)


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

- Reading/Writing files

### Resources Manager

- Loading/Unloading per folder + file type

### Game Module

- Game module loading at runtime in a DLL
- Code execution across DLL

### Editor Module

- Editor module loaded at runtime in a DLL
- Editor uses Qt for gui

### Debugging/Profiling

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

## ROADMAP FIRST PROTOTYPE 0.1.0 (January 2026)

### Editor (Qt)

- [x] Docking
- [x] Scene Tree panel
- [ ] Material editor panel
- [ ] Viewport panel
- [ ] Asset Browser
- [ ] Console
- [ ] Stats overlay
  - [x] Audio source count
  - [x] Minimal frame profiler (FPS, Draw Calls, Lights counts, etc...)
  - [ ] Hardware infos (CPU, GPU, RAM) (Name, Vendor, Usage...)

### Rendering

- [ ] Skyboxes + IBL
- [ ] Wireframe rendering
- [ ] Forward+ rendering
- [ ] Ligthmaps
- [ ] FBX importer (multiple meshes)
- [ ] Post processing effects shaders (SSAO, Bloom, Vignette, Tone Mapping, Film Grain, Chromatic Abberration)

#### Serialization

- [ ] Component serialization
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

## Credits/Dependencies

- Libraries/Projects :
  - [Qt](https://www.qt.io/)
  - [Qt imgui backend](https://github.com/seanchas116/qtimgui)
  - [Qt ads](https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System)
  - [GLFW](https://github.com/glfw/glfw)
  - [GLM](https://github.com/g-truc/glm)
  - [Jolt](https://github.com/jrouwe/JoltPhysics)
  - [IconFontCppHeaders](https://github.com/juliettef/IconFontCppHeaders)
  - [ImGui](https://github.com/ocornut/imgui)
  - [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo)
  - [freetype](https://github.com/freetype/freetype)
  - [glad](https://github.com/Dav1dde/glad)
  - [ufbx](https://github.com/ufbx/ufbx)
  - [json for c++](https://github.com/nlohmann/json)
  - [fmod](https://www.fmod.com/)

- Fonts :
  - [Lucide icons](https://lucide.dev/)
  - [Open Sans](https://fonts.google.com/specimen/Open+Sans)

- Sounds/Music :
  - [TownTheme.mp3](https://opengameart.org/content/town-theme-rpg)

- Models :
  - "Rubik's Cube" (<https://skfb.ly/6U7pp>) by RED2000 is licensed under Creative Commons Attribution (<http://creativecommons.org/licenses/by/4.0/>).
  - "Sponza" Model downloaded from Morgan McGuire's [Computer Graphics Archive](https://casual-effects.com/data)
