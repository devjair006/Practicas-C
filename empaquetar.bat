@echo off
REM Doble-clic para generar la version portable (.exe + dependencias + assets + zip)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0empaquetar.ps1"
pause
