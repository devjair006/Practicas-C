@echo off
echo =======================================
echo    COMPILANDO PROYECTO OPENGL
echo =======================================
echo.

:: Intentar buscar cmake en distintas ubicaciones de Visual Studio
set "CMAKE_EXE="

:: Visual Studio 2025 (v18)
if exist "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
    set "CMAKE_EXE=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    goto found
)

:: Visual Studio 2022 (v17)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
    set "CMAKE_EXE=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    goto found
)

:: CMake global (instalado aparte)
where cmake >nul 2>nul
if %errorlevel% equ 0 (
    set "CMAKE_EXE=cmake"
    goto found
)

echo [ERROR] No se encontro CMake en tu computadora.
echo Instala Visual Studio Community con las herramientas de C++,
echo o instala CMake desde https://cmake.org/download/
pause
exit /b 1

:found
echo CMake encontrado: %CMAKE_EXE%
echo.

echo [1/2] Preparando entorno de compilacion (X64)...
"%CMAKE_EXE%" -A x64 -B build_win -S .
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Fallo al configurar el proyecto.
    pause
    exit /b %errorlevel%
)

echo.
echo [2/2] Compilando codigo...
"%CMAKE_EXE%" --build build_win --config Release
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Fallo al compilar. Revisa los errores arriba.
    pause
    exit /b %errorlevel%
)

echo.
echo =======================================
echo    LANZANDO APLICACION...
echo =======================================
start "" ".\build_win\Release\app.exe"
