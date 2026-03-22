# 🎮 Prácticas de OpenGL en C++ (FreeGLUT)

Este repositorio contiene una **Wikipedia interactiva de GLUT** construida en C++ con OpenGL y FreeGLUT. Incluye ejemplos visuales de primitivas 3D, iluminación, texto, eventos de teclado/mouse y más.

---

## 📋 Requisitos Previos

### 🖥️ Windows
1. **Visual Studio 2022 Community** (o superior) con la carga de trabajo **"Desarrollo de escritorio con C++"** instalada.
2. **Visual Studio Code** (opcional, pero recomendado para editar cómodamente).

> [!NOTE]
> **NO necesitas instalar FreeGLUT, GLFW ni ninguna librería extra.** El repositorio ya incluye todos los archivos `.dll` y `.lib` necesarios para Windows dentro de las carpetas `freeglut-MSVC/` y `glfw-win64/`.

### 🍎 macOS / Linux
1. **Xcode Command Line Tools** (macOS):
   ```bash
   xcode-select --install
   ```
2. Instalar las dependencias con tu gestor de paquetes:
   ```bash
   # macOS (Homebrew)
   brew install freeglut glm

   # Linux (Debian/Ubuntu)
   sudo apt install freeglut3-dev libglm-dev
   ```

---

## 🚀 ¿Cómo compilar y correr?

### Forma 1: Doble clic (La más fácil - Solo Windows)
1. Clona o descarga el repositorio.
2. Haz **doble clic** en el archivo **`compilar_y_correr.bat`**.
3. Se abrirá una terminal negra que compilará todo automáticamente y al terminar abrirá la ventana de OpenGL.

### Forma 2: VS Code con Ctrl+Shift+B (Recomendado para desarrollar)
1. Abre la carpeta del proyecto en **VS Code**.
2. Abre una terminal dentro de VS Code (`Ctrl + ñ` o `Terminal > New Terminal`).
3. **Solo la primera vez**, ejecuta este comando para preparar el entorno de compilación:
   ```powershell
   & "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -B build_win -S .
   ```
   > [!TIP]
   > Si tu versión de Visual Studio es diferente (por ejemplo 2022 en lugar de 2025), la ruta del `cmake.exe` será distinta. Busca tu `cmake.exe` dentro de la carpeta de tu Visual Studio, o si tienes CMake instalado globalmente, simplemente usa:
   > ```powershell
   > cmake -B build_win -S .
   > ```

4. A partir de ahora, cada vez que quieras compilar y ejecutar, solo presiona:
   **`Ctrl + Shift + B`**
   Esto compilará tu código y abrirá la ventana automáticamente.

### Forma 3: Visual Studio 2022 directamente
1. Abre Visual Studio 2022.
2. Selecciona **"Abrir una carpeta local"** y elige la carpeta de este proyecto.
3. El editor detectará el `CMakeLists.txt` automáticamente.
4. Selecciona `app.exe` como objetivo y presiona el botón verde ▶️.

### Forma 4: macOS / Linux (Terminal)
1. Instala las dependencias indicadas arriba.
2. Abre la terminal en la carpeta del proyecto.
3. Compila y ejecuta:
   ```bash
   make
   ./app
   ```
   *(Usa `make clean` para limpiar los binarios y forzar una recompilación limpia).*

---

## 📂 Estructura del Proyecto

| Archivo / Carpeta | Descripción |
|-|-|
| `src/main.cpp` | Código principal con la Wikipedia interactiva de GLUT |
| `Plantilla_OpenGL.cpp` | Plantilla base limpia para nuevos proyectos con FreeGLUT |
| `Plantilla_Moderno.cpp` | Plantilla base para proyectos con GLFW + GLAD (OpenGL 3.3+) |
| `PlantillaguiadeGlut.cpp` | Copia completa de la guía interactiva para usar como referencia |
| `compilar_y_correr.bat` | Script de Windows que compila y ejecuta con un doble clic |
| `CMakeLists.txt` | Configuración de CMake para compilar en Windows |
| `Makefile` | Configuración para compilar en macOS/Linux |
| `freeglut-MSVC/` | Librería FreeGLUT precompilada para Windows |
| `glfw-win64/` | Librería GLFW precompilada para Windows |
| `include/` | Cabeceras de librerías extras (GLM, stb_image, GLAD) |
| `.vscode/` | Configuración de VS Code (atajos de compilación) |

---

## ⚠️ Importante sobre Git y GitHub
**NO subas las carpetas de compilación (`build/`, `build_win/`) ni ejecutables (`.exe`, `.o`) a tu repositorio.**
El `.gitignore` incluido ya se encarga de esto automáticamente.

---

## 📝 Notas Adicionales

> [!IMPORTANT]
> El Makefile compila `main.cpp` (C++). Si deseas compilar código en C puro, renombra tu archivo a `main.c` y cambia `$(CXX)` por `$(CC)` en el Makefile.

> [!TIP]
> **Makefile en Windows:** Para usar el comando `make` en Windows necesitas **MinGW-w64** o **Make for Windows** configurado en tu PATH. Sin embargo, el método recomendado en Windows es usar CMake (Ctrl+Shift+B o el archivo `.bat`).
