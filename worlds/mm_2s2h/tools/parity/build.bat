@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%~dp0"
cl /nologo /std:c++20 /EHsc /O1 /bigobj /W0 /MP /Ishim /Ibuild ^
    /I..\..\..\..\..\2s2h\mm /I..\..\..\..\..\2s2h\mm\2s2h ^
    /I..\..\..\..\..\2s2h\build\x64\vcpkg\installed\x64-windows-static\include ^
    harness_main.cpp build\regions\*.cpp /Fe:build\parity.exe /Fo:build\ > build\cl.log 2>&1
if errorlevel 1 (
    type build\cl.log
    exit /b 1
)
