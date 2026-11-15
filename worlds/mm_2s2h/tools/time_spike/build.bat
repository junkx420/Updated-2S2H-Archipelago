@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /std:c++20 /EHsc /O2 /W3 harness.cpp /Fe:build\harness.exe /Fo:build\ >build\cl.log 2>&1
type build\cl.log
