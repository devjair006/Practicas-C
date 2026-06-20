# script para automatizar la creación del ejecutable portable y su distribución.

$ErrorActionPreference = "Stop"

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "   CREACIÓN DE PAQUETE DE DISTRIBUCIÓN STANDALONE" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan
Write-Host ""

# 1. Compilar el juego en modo Release usando CMake
Write-Host "[1/5] Buscando CMake e iniciando compilación..." -ForegroundColor Yellow

$cmakeExe = "cmake"
$vs2025Path = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$vs2022Path = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

if (Test-Path $vs2025Path) {
    $cmakeExe = $vs2025Path
} elseif (Test-Path $vs2022Path) {
    $cmakeExe = $vs2022Path
} else {
    # Verificar si cmake está en el PATH
    $checkPath = Get-Command cmake -ErrorAction SilentlyContinue
    if ($null -eq $checkPath) {
        Write-Error "No se encontró CMake. Asegúrate de tener instalado Visual Studio con desarrollo de C++ o CMake en el PATH."
    }
}

Write-Host "Usando CMake en: $cmakeExe" -ForegroundColor Gray

if (-not (Test-Path "build_win")) {
    Write-Host "Configurando el proyecto con CMake..." -ForegroundColor Gray
    & $cmakeExe -A x64 -B build_win -S .
}
& $cmakeExe --build build_win --config Release

# Verificar que el ejecutable existe
$exePath = "build_win\Release\app.exe"
if (-not (Test-Path $exePath)) {
    Write-Error "No se pudo encontrar el ejecutable generado en $exePath"
}

# 2. Limpiar y recrear carpeta dist_game
Write-Host "[2/5] Limpiando carpeta de distribución anterior (dist_game)..." -ForegroundColor Yellow
if (Test-Path "dist_game") {
    Remove-Item -Path "dist_game" -Recurse -Force
}
New-Item -ItemType Directory -Path "dist_game" | Out-Null

# 3. Copiar ejecutable y renombrarlo para mayor claridad
Write-Host "[3/5] Copiando ejecutable..." -ForegroundColor Yellow
Copy-Item -Path $exePath -Destination "dist_game\Proyecto-Confidencial.exe"

# 4. Copiar todas las DLLs necesarias
Write-Host "[4/5] Copiando librerías dinámicas (.dll)..." -ForegroundColor Yellow
$dlls = Get-ChildItem -Path "build_win\Release\*.dll"
foreach ($dll in $dlls) {
    Write-Host "  -> Copiando $($dll.Name)" -ForegroundColor Gray
    Copy-Item -Path $dll.FullName -Destination "dist_game\"
}

# 5. Copiar la carpeta de assets completa
Write-Host "[5/5] Copiando carpeta de assets..." -ForegroundColor Yellow
if (Test-Path "assets") {
    Copy-Item -Path "assets" -Destination "dist_game\assets" -Recurse
} else {
    Write-Warning "No se encontró la carpeta 'assets'. El juego podría no funcionar sin recursos."
}

# 6. Comprimir en un archivo ZIP
$zipName = "Proyecto-Confidencial_Release.zip"
Write-Host ""
Write-Host "Comprimiendo todo en $zipName..." -ForegroundColor Green
if (Test-Path $zipName) {
    Remove-Item -Path $zipName -Force
}
Compress-Archive -Path "dist_game" -DestinationPath $zipName -Force

Write-Host ""
Write-Host "¡Proceso terminado con éxito!" -ForegroundColor Green
Write-Host "Puedes compartir '$zipName' con cualquier persona para que juegue." -ForegroundColor Green
Write-Host "Para jugar, solo deben extraer el ZIP y abrir 'Proyecto-Confidencial.exe'." -ForegroundColor Green
