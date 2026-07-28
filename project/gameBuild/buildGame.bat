@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0.."

echo [Game Builder] Setting up Visual Studio build environment...

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

echo [Game Builder] Calling: "%VS_PATH%"
call "%VS_PATH%"

echo [Game Builder] Building CG2_project.sln with Configuration=Debug, Platform=x64, PlatformToolset=v143...
msbuild CG2_project.sln -property:Configuration=Debug -property:Platform=x64 -property:PlatformToolset=v143 -maxCpuCount

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Compilation failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo [Game Builder] Successfully built game!
exit /b 0
