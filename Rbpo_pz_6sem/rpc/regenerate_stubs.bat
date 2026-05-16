@echo off
setlocal
cd /d "%~dp0"

echo Regenerating RPC stubs from ServiceControl.idl ...
midl /nologo /W1 /robust ServiceControl.idl
if errorlevel 1 exit /b 1

echo.
echo Copy generated stubs to platform-specific names:
echo   ServiceControl_c.c  -^> ServiceControl_c_x64.c (x64)
echo   ServiceControl_s.c  -^> ServiceControl_s_x64.c (x64)
echo Repeat with midl /env win32 and /env arm64 for other targets.
echo.
echo Or open a "x64 Native Tools" / "ARM64 Native Tools" prompt and run midl per platform.
