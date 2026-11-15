@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d %~dp0
cl /nologo /std:c++20 /EHsc /O1 /bigobj /W0 /MP /Ishim /Ibuild /I..\..\..\..\..\2s2h\mm\2s2h harness_main.cpp build\regions\*.cpp /Fe:build\parity.exe /Fo:build\ > build\cl.log 2>&1
findstr /C:"error" build\cl.log
