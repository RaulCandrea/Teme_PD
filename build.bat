@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

echo [1/3] Compilare registry_enum.cpp ...
cl /nologo /W4 /EHsc registry_enum.cpp advapi32.lib
echo.

echo [2/3] Compilare device_info.cpp ...
cl /nologo /W4 /EHsc device_info.cpp setupapi.lib advapi32.lib
echo.

echo [3/3] Compilare hello_service.cpp ...
cl /nologo /W4 /EHsc hello_service.cpp advapi32.lib
echo.
echo Done.
