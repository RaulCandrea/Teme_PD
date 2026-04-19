@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

echo [1/2] Compilare registry_enum.cpp ...
cl /nologo /W4 /EHsc registry_enum.cpp advapi32.lib
echo.

echo [2/2] Compilare device_info.cpp ...
cl /nologo /W4 /EHsc device_info.cpp setupapi.lib advapi32.lib
echo.
echo Done.
