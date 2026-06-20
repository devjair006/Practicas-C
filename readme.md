# 👁️ Confidential Project: Full Copy

A **3D first-person exploration and horror game** built from scratch in **C++** using **OpenGL 3.3 (Core Profile)**, **GLFW**, **GLAD**, and a set of powerful low-level libraries.

You wake up in an abandoned and silent laboratory. Your only companion is a dim flashlight. The system warns of a replication anomaly: the copy no longer obeys and is learning your movements...

---

## 🚀 Key Features

### 🎮 Gameplay Mechanics (Survival Horror)
* **Stamina and Exhaustion:** Running (`Left Shift`) drains your energy. If it is completely depleted, you will enter an exhausted state that requires recovery before you can run again.
* **Organic Movement (Head Bobbing):** Dynamic camera simulation when walking or running for complete immersion.
* **Dynamic Flashlight (Spotlight):** A realistic first-person flashlight attached to the player's view, with realistic quadratic attenuation and intensity.
* **Keycard System:** Collect the **Yellow Keycard (Level 1)** and the **Red Keycard (Level 2)** to unlock new sections of the map with fluid real-time 3D animations for the double metal doors.
* **Alternate Dimension (Distortion):** Find the **3 hidden batteries**, reactivate the power console in the main laboratory, and cross the threshold. The environment will lose stability, changing to a blood-red tone, with temporal ripple shaders and dynamic vignetting.
* **The Mimicking Threat:** An entity lurks in the darkness. It follows the *Weeping Angel* rule: it only moves when you stop looking directly at it. If it reaches you, you will be replaced.
* **Interactive Lore:** Examine notes, crew logs, and terminal screens with an interactive on-screen typewriter viewer.

### 🛠️ Development Tools & Interface (Dear ImGui)
* The project features a complete real-time debugging suite that can be toggled by pressing `TAB`.
* **Collision Viewer (AABB):** Draws collision boxes for walls and bathroom props in real-time.
* **Prop Manipulator:** Live-adjust the position, rotation, and scale of all 3D models and lights on the map.
* **Animation Tester:** Allows playing and inspecting the skeletal (bone) animations of the glTF model (`gnome.glb`).

### 📦 Graphics Engine & Multimedia
* **Complex 3D Model Loading:** Integration with **Assimp** to process complex meshes (`.obj` files such as laptops, doors, cables, etc.) and `.gltf`/`.glb` files with skeletal (bone) animation support.
* **Immersive Audio:** Integration of **miniaudio** to play looping background music (`music.mp3`) and spatial/positional sound effects (`collect.wav`, `click.wav`, `start.wav`).
* **Advanced Lighting:** Ambient and diffuse lighting based on multiple dynamic light sources (Point Lights) with a smooth attenuation formula similar to *Unreal Engine*, in addition to the main flashlight (Spotlight).

---

## 🎹 Game Controls

| Key | Action |
| :--- | :--- |
| **`W` / `A` / `S` / `D`** | Move through the laboratory |
| **`Mouse`** | Rotate camera (Look around) |
| **`Left Shift`** | Sprint (Run while consuming stamina) |
| **`E`** | Interact (Open drawers/doors, pick up keycards/batteries, use console, read notes) |
| **`F`** | Toggle Flashlight On / Off |
| **`TAB`** | Lock/Unlock cursor to interact with the debug panel (ImGui) |
| **`ESC`** | Exit game (or close reading document) |
| **`SPACE` / `ENTER`** | Start the game from the main menu |

---

## 📂 Project Structure

```text
Practicas-C/
├── assets/                  # 3D Models (.glb, .obj), textures, and audio files
│   ├── gnome.glb            # Animated enemy model
│   ├── Bano.glb, Urinario...# 3D props for the bathroom environment
│   ├── laptop.obj, cables.obj# Auxiliary 3D models in OBJ format
│   ├── music.mp3, click.wav # Soundtrack and sound effects
│   └── *.png, *.jpg         # Map and skybox textures
├── include/                 # External headers (GLM, stb_image, miniaudio, ImGui, GLAD)
├── src/                     # Application source code
│   ├── headers/             # Custom header files (.h)
│   │   ├── game_state.h     # Global state, variables, and map
│   │   ├── gameplay.h       # Collision and interaction prototypes
│   │   └── ...
│   ├── shaders/             # GLSL vertex and fragment shaders
│   │   ├── vertex.vert      # Dimensional deformation and bone matrix
│   │   └── fragment.frag    # Spotlight, zone lights, and distortion filters
│   ├── main.cpp             # Main rendering loop and OpenGL initialization
│   ├── gameplay.cpp         # Game logic, AABB collisions, and movement
│   └── game_state.cpp       # 50x50 map definition and entities
├── CMakeLists.txt           # CMake project configuration (compiles on Windows/macOS/Linux)
├── compilar_y_correr.bat    # Automation script for Windows (CMake + VS Build)
└── Makefile                 # Native compilation configuration for Unix (macOS/Linux)
```

---

## 🛠️ System Requirements

### Windows
1. **Visual Studio 2022 or 2025** with the **"Desktop development with C++"** workload installed.
2. **CMake** (includes support for auto-detecting default MSVC paths).

> [!NOTE]
> **The project includes all its precompiled dependencies for Windows.** There is no need to manually install GLFW or FreeGLUT. The CMake script will automatically copy the required DLLs (`freeglut.dll`, `glfw3.dll`, and the Assimp DLL) to the output folder during compilation.

### macOS / Linux
1. Compilation tools (`clang` or `gcc`, `make`, and `cmake`).
2. Install OpenGL, GLFW3, FreeGLUT, and Assimp dependencies through your preferred package manager:
   ```bash
   # macOS (Homebrew)
   brew install glfw freeglut assimp glm
   
   # Ubuntu/Debian
   sudo apt update
   sudo apt install build-essential cmake libglfw3-dev libglut3-dev libassimp-dev libglm-dev
   ```

---

## ⚙️ How to Build and Run?

> [!IMPORTANT]
> If you have just cloned the repo (or the submodule commit changed), initialize/update submodules before building:
> ```bash
> git submodule update --init --recursive
> ```

### Method 1: Automated Script (Recommended - Windows Only)
1. **Double-click** the `compilar_y_correr.bat` file in the project root.
2. The script will detect the installation of Visual Studio 2022/2025 or your global CMake, clean previous builds, configure CMake, and build the release version (`Release`).
3. Upon successful completion, the game will launch automatically.

### Method 2: VS Code with CMake Tools (Cross-platform)
1. Open this folder in **VS Code**.
2. Install the **CMake Tools** extension if you don't have it yet.
3. Press `Ctrl + Shift + P`, type `CMake: Configure`, and select your compiler.
4. Press the **Build** button on the bottom bar (or press `F7`), and then run with the **Play** button (or `Shift + F5`).

### Method 3: Directly via Visual Studio
1. Open Visual Studio.
2. Select **"Open a local folder"** and choose this repository's folder.
3. Visual Studio will automatically detect the `CMakeLists.txt` file and begin indexing dependencies.
4. Select `app.exe` from the startup target list and click the green play button.

---

## ⚠️ Tips for Developers

> [!IMPORTANT]
> **Collision Consistency:** Player collisions use a spherical radius of `0.25f` against the AABB bounding boxes of the bathroom props loaded by Assimp (`banoGLTF->GetWorldAABB(model)`). Ensure that the transformations and scales applied when rendering a prop exactly match those calculated in the `checkCollision` function in [gameplay.cpp](file:///C:/c++/Practicas-C/src/gameplay.cpp) to prevent misaligned walls and invisible obstacles.

> [!TIP]
> **Resource Management (Assets):** If you add new 3D models or audio files, place them inside the `assets/` directory. CMake will copy binary dependency files, but the executable searches for resources relative to the working execution directory (which should normally point to the repository root, where the `assets/` folder is located).
