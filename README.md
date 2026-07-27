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
</p>

## Screenshots

<img src=".github/ScreenShot0.png"/>
<img src=".github/ScreenShot1.png"/>

## How to use (Linux)

// WIP

## How to use (Windows)

### Build tools :
- CMake 3.28.2 or later
- C++ 17 Compiler (GCC MinGW recommended)

### Proprietary Dependencies :
- FMOD Core API 2.03.14

### Editor Fonts : (Place both in src/editor/gui/fonts/)
- [OpenSans-Regular.ttf](https://github.com/googlefonts/opensans)
- [lucide.ttf](https://unpkg.com/lucide-static@latest/font/lucide.ttf)


### How to build :

Place the fonts (.ttf) inside their folder (editor_resources/fonts/)

Run build.bat or build.sh (depending on your OS)

This will compile everything from root : submodules, the engine, the editor, and the game module (loaded with the game app)

Drop fmod.dll and glfw3.dll inside the build folder

Drop engine_resources folder and editor_resources folder inside build directory

### How to run :

This runs the editor, loads the game module, opens the project at "project path" and uses open gl core as the rendering api
```bash
./PulseEditor.exe --game libGameModule.dll --project ..\\test_project\\test_project.json --api opengl
```

## Credits/Dependencies

- Libraries/Projects :
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
  - [ImViewGuizmo](https://github.com/Ka1serM/ImViewGuizmo)

- Fonts :
  - [Lucide icons](https://lucide.dev/)
  - [Open Sans](https://fonts.google.com/specimen/Open+Sans)

- Sounds/Music :
  - [TownTheme.mp3](https://opengameart.org/content/town-theme-rpg)

- Models :
  - "Rubik's Cube" (<https://skfb.ly/6U7pp>) by RED2000 is licensed under Creative Commons Attribution (<http://creativecommons.org/licenses/by/4.0/>).
  - "Sponza" Model downloaded from Morgan McGuire's [Computer Graphics Archive](https://casual-effects.com/data)
  - "Cerberus" Gun model [Andrew Maximov](https://artisaverb.info/PBT.html) 

- Textures :
  - [Qwantani Afternoon (Pure Sky)](https://polyhaven.com/a/qwantani_afternoon_puresky)
