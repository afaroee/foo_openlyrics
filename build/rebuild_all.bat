@echo off
set VS_PATH=C:\Program Files\Microsoft Visual Studio\18\Community
set DEV_CMD=%VS_PATH%\Common7\Tools\VsDevCmd.bat
set MSBUILD_EXE=%VS_PATH%\MSBuild\Current\Bin\MSBuild.exe

echo Initializing VS environment...
call "%DEV_CMD%"

echo Rebuilding Win32 Release (foo_openlyrics)...
"%MSBUILD_EXE%" foo_openlyrics.sln /p:Configuration=Release /p:Platform=x86 /p:PlatformToolset=v145 /p:WindowsTargetPlatformVersion=10.0.26100.0 /t:foo_openlyrics:Rebuild > rebuild_win32.log 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Win32 build failed.
)

echo Rebuilding x64 Release (foo_openlyrics)...
"%MSBUILD_EXE%" foo_openlyrics.sln /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v145 /p:WindowsTargetPlatformVersion=10.0.26100.0 /t:foo_openlyrics:Rebuild > rebuild_x64.log 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo x64 build failed.
)

echo Done.
