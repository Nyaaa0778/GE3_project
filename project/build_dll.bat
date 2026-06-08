@echo off
setlocal enabledelayedexpansion

echo [DLL Builder] Setting up Visual Studio build environment...

set "VS_PATH="
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
) else (
    for /d %%d in ("C:\Program Files\Microsoft Visual Studio\*") do (
        for /d %%e in ("%%d\*") do (
            if exist "%%e\VC\Auxiliary\Build\vcvars64.bat" (
                set "VS_PATH=%%e\VC\Auxiliary\Build\vcvars64.bat"
            )
        )
    )
)

if "%VS_PATH%"=="" (
    echo [ERROR] Visual Studio vcvars64.bat not found.
    exit /b 1
)

echo [DLL Builder] Calling: "%VS_PATH%"
call "%VS_PATH%"

if "%1"=="debug" (
    echo [DLL Builder] Building DEBUG configuration - MTd, Od
    set "OPTS=/MTd /Od /Zi"
) else (
    echo [DLL Builder] Building DEVELOPMENT/RELEASE configuration - MT, O2
    set "OPTS=/MT /O2"
)

echo [DLL Builder] Compiling DebugUI.dll...

cl.exe /LD %OPTS% /EHsc /DUSE_IMGUI /I"externals/imgui" /I"game/scenes" ^
    "game/scenes/DebugUI.cpp" ^
    "externals/imgui/imgui.cpp" ^
    "externals/imgui/imgui_draw.cpp" ^
    "externals/imgui/imgui_widgets.cpp" ^
    "externals/imgui/imgui_tables.cpp" ^
    /link /OUT:DebugUI.dll

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Compilation failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo [DLL Builder] Successfully built: DebugUI.dll
exit /b 0
