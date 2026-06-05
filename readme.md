# 👁️ Proyecto Confidencial: Copia Completa

Un videojuego de **exploración y terror en primera persona en 3D** construido desde cero en **C++** utilizando **OpenGL 3.3 (Core Profile)**, **GLFW**, **GLAD** y un conjunto de librerías potentes de bajo nivel.

Te despiertas en un laboratorio abandonado y silencioso. Tu única compañía es una linterna mortecina. El sistema advierte de una anomalía en la replicación: la copia ya no obedece y está aprendiendo tus movimientos...

---

## 🚀 Características Principales

### 🎮 Mecánicas de Juego (Survival Horror)
* **Estamina y Agotamiento:** Correr (`Shift Izquierdo`) drena tu energía. Si se agota por completo, entrarás en un estado de cansancio que requiere recuperación antes de poder correr de nuevo.
* **Movimiento Orgánico (Head Bobbing):** Simulación dinámica de la cámara al caminar o correr para una inmersión completa.
* **Linterna Dinámica (Spotlight):** Una linterna realista en primera persona acoplada a la vista del jugador, con atenuación e intensidad cuadrática realista.
* **Sistema de Tarjetas de Acceso (Keycards):** Recolecta la **Tarjeta Amarilla (Nivel 1)** y la **Tarjeta Roja (Nivel 2)** para desbloquear nuevas secciones del mapa con animaciones 3D fluidas en tiempo real para las puertas dobles de metal.
* **Dimensión Alterna (Distorsión):** Encuentra las **3 baterías ocultas**, reactiva la consola de energía en el laboratorio principal y cruza el umbral. El entorno perderá estabilidad, cambiando a un tono rojo sangre, con shaders de ondulación temporal y viñeteado dinámico.
* **La Amenaza Imitadora:** Un ente acecha en la oscuridad. Sigue la regla del *Ángel Llorón*: solo se mueve cuando dejas de mirarlo directamente. Si te alcanza, serás reemplazado.
* **Lore Interactivo:** Examina notas, logs de la tripulación y pantallas de terminales con un visor typewriter interactivo en pantalla.

### 🛠️ Herramientas de Desarrollo e Interfaz (Dear ImGui)
* El proyecto cuenta con una suite completa de depuración en tiempo real que se puede desplegar presionando `TAB`.
* **Visor de Colisiones (AABB):** Dibuja las cajas de colisión para las paredes y los props del baño en tiempo real.
* **Manipulador de Props:** Ajusta en vivo la posición, rotación y escala de todos los modelos 3D y luces del mapa.
* **Probador de Animaciones:** Permite reproducir e inspeccionar las animaciones óseas del modelo glTF (`gnome.glb`).

### 📦 Motor Gráfico & Multimedia
* **Carga de Modelos 3D Complejos:** Integración con **Assimp** para procesar mallas complejas (archivos `.obj` como laptops, puertas, cables, etc.) y archivos `.gltf`/`.glb` con soporte de animación de esqueletos (huesos).
* **Audio Inmersivo:** Integración de **miniaudio** para reproducir música ambiental en bucle (`music.mp3`) y reproducir efectos de sonido posicionales (`collect.wav`, `click.wav`, `start.wav`).
* **Iluminación Avanzada:** Iluminación ambiental y difusa basada en múltiples fuentes de luz dinámicas (Point Lights) con una fórmula de atenuación suave similar a la de *Unreal Engine*, además de la linterna principal (Spotlight).

---

## 🎹 Controles del Juego

| Tecla | Acción |
| :--- | :--- |
| **`W` / `A` / `S` / `D`** | Moverse por el laboratorio |
| **`Mouse`** | Girar la cámara (Mirar alrededor) |
| **`Shift Izquierdo`** | Sprintar (Correr consumiendo estamina) |
| **`E`** | Interactuar (Abrir cajones/puertas, recoger tarjetas/baterías, usar consola, leer notas) |
| **`F`** | Encender / Apagar la Linterna |
| **`TAB`** | Bloquear/Desbloquear cursor para interactuar con el panel de depuración (ImGui) |
| **`ESC`** | Salir del juego (o cerrar documento en lectura) |
| **`SPACE` / `ENTER`** | Iniciar el juego desde el menú principal |

---

## 📂 Estructura del Proyecto

```text
Practicas-C/
├── assets/                  # Modelos 3D (.glb, .obj), texturas y archivos de audio
│   ├── gnome.glb            # Modelo del enemigo animado
│   ├── Bano.glb, Urinario...# Props 3D para el entorno del baño
│   ├── laptop.obj, cables.obj# Modelos 3D auxiliares en formato OBJ
│   ├── music.mp3, click.wav # Banda sonora y efectos de sonido
│   └── *.png, *.jpg         # Texturas del mapa y del cielo
├── include/                 # Cabeceras externas (GLM, stb_image, miniaudio, ImGui, GLAD)
├── src/                     # Código fuente de la aplicación
│   ├── headers/             # Archivos de cabecera custom (.h)
│   │   ├── game_state.h     # Estado global, variables y mapa
│   │   ├── gameplay.h       # Prototipos de colisiones e interacción
│   │   └── ...
│   ├── shaders/             # Shaders GLSL de vértices y fragmentos
│   │   ├── vertex.vert      # Deformación dimensional y matriz de huesos
│   │   └── fragment.frag    # Spotlight, luces de zona y filtros de distorsión
│   ├── main.cpp             # Bucle principal de renderizado e inicialización de OpenGL
│   ├── gameplay.cpp         # Lógica de juego, colisiones AABB y movimiento
│   └── game_state.cpp       # Definición del mapa 50x50 y entidades
├── CMakeLists.txt           # Configuración del proyecto de CMake (compila en Windows/macOS/Linux)
├── compilar_y_correr.bat    # Script de automatización para Windows (CMake + VS Build)
└── Makefile                 # Configuración de compilación nativa para Unix (macOS/Linux)
```

---

## 🛠️ Requisitos del Sistema

### Windows
1. **Visual Studio 2022 o 2025** con la carga de trabajo **"Desarrollo de escritorio con C++"** instalada.
2. **CMake** (se incluye soporte para autodetectar las rutas por defecto de MSVC).

> [!NOTE]
> **El proyecto incluye todas sus dependencias precompiladas para Windows.** No es necesario instalar GLFW ni FreeGLUT manualmente. El script de CMake copiará automáticamente las DLLs necesarias (`freeglut.dll`, `glfw3.dll` y la DLL de Assimp) a la carpeta de salida al compilar.

### macOS / Linux
1. Herramientas de compilación (`clang` o `gcc`, `make` y `cmake`).
2. Instalar las dependencias de OpenGL, GLFW3, FreeGLUT y Assimp a través de tu gestor de paquetes de preferencia:
   ```bash
   # macOS (Homebrew)
   brew install glfw freeglut assimp glm
   
   # Ubuntu/Debian
   sudo apt update
   sudo apt install build-essential cmake libglfw3-dev libglut3-dev libassimp-dev libglm-dev
   ```

---

## ⚙️ ¿Cómo Compilar y Ejecutar?

> [!IMPORTANT]
> Si acabas de clonar el repo (o cambió el commit del submódulo), inicializa/actualiza submódulos antes de compilar:
> ```bash
> git submodule update --init --recursive
> ```

### Método 1: Script Automatizado (Recomendado - Solo Windows)
1. Haz **doble clic** sobre el archivo `compilar_y_correr.bat` en la raíz del proyecto.
2. El script detectará la instalación de Visual Studio 2022/2025 o tu CMake global, limpiará compilaciones previas, configurará CMake y compilará la versión de lanzamiento (`Release`).
3. Al finalizar con éxito, el videojuego se abrirá automáticamente.

### Método 2: VS Code con CMake Tools (Multiplataforma)
1. Abre esta carpeta en **VS Code**.
2. Instala la extensión **CMake Tools** si aún no la tienes.
3. Presiona `Ctrl + Shift + P`, escribe `CMake: Configure` y selecciona tu compilador.
4. Presiona el botón **Build** en la barra inferior (o presiona `F7`), y luego ejecuta con el botón **Play** (o `Shift + F5`).

### Método 3: Visual Studio Directamente
1. Abre Visual Studio.
2. Selecciona **"Abrir una carpeta local"** y elige la carpeta de este repositorio.
3. Visual Studio detectará el archivo `CMakeLists.txt` de manera automática y comenzará la indexación de dependencias.
4. Selecciona `app.exe` en la lista de objetivos de inicio y presiona el botón verde de reproducción.

---

## ⚠️ Consejos para Desarrolladores

> [!IMPORTANT]
> **Consistencia de Colisiones:** Las colisiones del jugador usan un radio esférico de `0.25f` contra las cajas delimitadoras AABB de los props del baño cargados por Assimp (`banoGLTF->GetWorldAABB(model)`). Asegúrate de que las transformaciones y escalas aplicadas al renderizar un prop coincidan exactamente con las calculadas en la función `checkCollision` en `src/gameplay.cpp` para evitar paredes y obstáculos invisibles desalineados.

> [!TIP]
> **Gestión de Recursos (Assets):** Si agregas nuevos modelos 3D o archivos de audio, colócalos dentro del directorio `assets/`. CMake copiará los archivos binarios de dependencias, pero el ejecutable busca los recursos de manera relativa al directorio de trabajo de ejecución (el cual normalmente debe apuntar a la raíz del repositorio, donde se encuentra la carpeta `assets/`).
