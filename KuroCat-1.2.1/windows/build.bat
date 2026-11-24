@echo off
setlocal
cd /d "%~dp0"

echo ==> Building KuroCat.exe (Windows native C++17 / Win32 + GDI+ + Raw Input)...

where cl >nul 2>nul
if %ERRORLEVEL%==0 (
    rc /nologo /fo app.res app.rc
    if errorlevel 1 exit /b 1
    cl /nologo /std:c++17 /O2 /MT /EHsc /utf-8 /DWINVER=0x0A00 /D_WIN32_WINNT=0x0A00 ^
       /W3 src\main.cpp app.res /Fe:KuroCat.exe ^
       /link /SUBSYSTEM:WINDOWS /MANIFEST:NO ^
       user32.lib gdi32.lib gdiplus.lib shell32.lib advapi32.lib ole32.lib
    if errorlevel 1 exit /b 1
    del /q main.obj app.res 2>nul
    echo ==> Built successfully with MSVC: %~dp0KuroCat.exe
    exit /b 0
)

where g++ >nul 2>nul
if %ERRORLEVEL%==0 (
    windres app.rc -O coff -o app-res.o
    if errorlevel 1 exit /b 1
    g++ -std=c++17 -O2 -mwindows -municode -DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00 -static ^
        src\main.cpp app-res.o -o KuroCat.exe ^
        -luser32 -lgdi32 -lgdiplus -lshell32 -ladvapi32 -lole32
    if errorlevel 1 exit /b 1
    del /q app-res.o 2>nul
    echo ==> Built successfully with MinGW: %~dp0KuroCat.exe
    exit /b 0
)

echo [ERROR] Neither MSVC (cl.exe) nor MinGW (g++.exe) found in PATH.
echo Please run from "x64 Native Tools Command Prompt for VS" or install Visual Studio Build Tools.
exit /b 1
