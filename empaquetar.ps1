# =====================================================================
#  EMPAQUETADOR PORTABLE - Copia Confidencial (OpenGL / C++)
#  Genera una carpeta y un .zip que corren en cualquier PC Windows x64
#  sin necesidad de instalar nada (compilador, Visual Studio, etc.)
# =====================================================================

$ErrorActionPreference = "Stop"
$ROOT = Split-Path -Parent $MyInvocation.MyCommand.Definition
Set-Location $ROOT

$NombreApp   = "CopiaConfidencial"
$DistDir     = Join-Path $ROOT "dist\$NombreApp"
$BuildDir    = Join-Path $ROOT "build_win"
$ReleaseDir  = Join-Path $BuildDir "Release"

Write-Host "=======================================" -ForegroundColor Cyan
Write-Host "   EMPAQUETANDO PROYECTO OPENGL"        -ForegroundColor Cyan
Write-Host "=======================================" -ForegroundColor Cyan

# ---------------------------------------------------------------------
# 1) Localizar CMake (Visual Studio 2025/2022 o instalacion global)
# ---------------------------------------------------------------------
$cmakeCandidates = @(
    "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
)
$CMAKE = $cmakeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $CMAKE) {
    $g = Get-Command cmake -ErrorAction SilentlyContinue
    if ($g) { $CMAKE = $g.Source }
}
if (-not $CMAKE) {
    Write-Host "[ERROR] No se encontro CMake. Instala Visual Studio Community con C++ o CMake." -ForegroundColor Red
    exit 1
}
Write-Host "CMake: $CMAKE" -ForegroundColor DarkGray

# ---------------------------------------------------------------------
# 2) Compilar version Release
# ---------------------------------------------------------------------
Write-Host "`n[1/4] Configurando y compilando (Release x64)..." -ForegroundColor Yellow
& $CMAKE -A x64 -B $BuildDir -S $ROOT
if ($LASTEXITCODE -ne 0) { Write-Host "[ERROR] Fallo la configuracion." -ForegroundColor Red; exit 1 }
& $CMAKE --build $BuildDir --config Release
if ($LASTEXITCODE -ne 0) { Write-Host "[ERROR] Fallo la compilacion." -ForegroundColor Red; exit 1 }

$exe = Join-Path $ReleaseDir "app.exe"
if (-not (Test-Path $exe)) { Write-Host "[ERROR] No se genero app.exe." -ForegroundColor Red; exit 1 }

# ---------------------------------------------------------------------
# 3) Preparar carpeta portable
# ---------------------------------------------------------------------
Write-Host "`n[2/4] Creando carpeta portable..." -ForegroundColor Yellow
if (Test-Path $DistDir) { Remove-Item $DistDir -Recurse -Force }
New-Item -ItemType Directory -Path $DistDir -Force | Out-Null

# 3a) Ejecutable
Copy-Item $exe (Join-Path $DistDir "app.exe") -Force

# 3b) DLLs propias del proyecto (raiz + salida de build)
$dllSources = @(
    "$ReleaseDir\assimp-vc143-mt.dll",
    "$ROOT\assimp-vc143-mt.dll",
    "$ROOT\freeglut.dll",
    "$ROOT\glfw3.dll",
    "$ROOT\poly2tri.dll",
    "$ROOT\minizip.dll",
    "$ROOT\z.dll",
    "$ROOT\kubazip.dll",
    "$ROOT\pugixml.dll"
)
$copiadas = @{}
foreach ($d in $dllSources) {
    if (Test-Path $d) {
        $name = Split-Path $d -Leaf
        if (-not $copiadas.ContainsKey($name)) {
            Copy-Item $d (Join-Path $DistDir $name) -Force
            $copiadas[$name] = $true
        }
    }
}

# 3c) Runtime de Visual C++ (para que corra sin instalar el redistribuible)
$runtime = @("MSVCP140.dll","VCRUNTIME140.dll","VCRUNTIME140_1.dll")
foreach ($r in $runtime) {
    $src = Join-Path $env:WINDIR "System32\$r"
    if (Test-Path $src) { Copy-Item $src (Join-Path $DistDir $r) -Force }
    else { Write-Host "  [aviso] No se encontro $r en System32" -ForegroundColor DarkYellow }
}

# 3d) Recursos: el .exe los busca por ruta relativa (assets\... y src\shaders\...)
Write-Host "[3/4] Copiando recursos (assets + shaders)... puede tardar (~830 MB)" -ForegroundColor Yellow
Copy-Item (Join-Path $ROOT "assets") (Join-Path $DistDir "assets") -Recurse -Force
New-Item -ItemType Directory -Path (Join-Path $DistDir "src\shaders") -Force | Out-Null
Copy-Item (Join-Path $ROOT "src\shaders\*") (Join-Path $DistDir "src\shaders") -Recurse -Force

# 3e) Lanzador amigable (asegura el directorio de trabajo correcto)
$launcher = @"
@echo off
cd /d "%~dp0"
start "" "app.exe"
"@
Set-Content -Path (Join-Path $DistDir "JUGAR.bat") -Value $launcher -Encoding ascii

# ---------------------------------------------------------------------
# 4) Comprimir en ZIP
# ---------------------------------------------------------------------
Write-Host "`n[4/4] Comprimiendo en ZIP..." -ForegroundColor Yellow
$zip = Join-Path $ROOT "dist\$NombreApp.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path $DistDir -DestinationPath $zip -CompressionLevel Optimal

Write-Host "`n=======================================" -ForegroundColor Green
Write-Host "   LISTO" -ForegroundColor Green
Write-Host "=======================================" -ForegroundColor Green
Write-Host "Carpeta portable : $DistDir"
Write-Host "ZIP distribuible : $zip"
Write-Host "`nPara jugar en otra PC: copia la carpeta (o descomprime el ZIP) y ejecuta JUGAR.bat o app.exe"
