@echo off
cd /d %~dp0..
echo [INFERNUS] Compilando proyecto...

cmake -S . -B build -G "MinGW Makefiles"
if %errorlevel% neq 0 (
    echo [ERROR] Fallo en la configuracion de CMake.
    pause
    exit /b %errorlevel%
)

mingw32-make -C build -j8
if %errorlevel% neq 0 (
    echo [ERROR] Fallo en la compilacion.
    pause
    exit /b %errorlevel%
)

echo [OK] Build exitoso. Ejecutando Infernus...
build\INFERNUS.exe
