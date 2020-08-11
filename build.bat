@echo off

::------------------------------
::
:: Relase / Debug
::
::------------------------------
SET /A RELEASE_BUILD=0


::------------------------------
::
:: Environment Settings
:: App name, architecture
::
::------------------------------
SET SCRIPT_DIR=%cd%
SET SOURCE_DIR=%SCRIPT_DIR%\src
SET INCLUDE_DIR=%SCRIPT_DIR%\include
SET BIN_DIR=%SCRIPT_DIR%\bin
SET APP_NAME=dx12
SET APP_ARCH=x64


::------------------------------
::
:: Compile Engine
:: Requires Visual Studio 2019
::
::------------------------------
SET VC_VARS_2019="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
SET VC_VARS_2017="C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat"

where cl >nul 2>nul
IF EXIST %VC_VARS_2019% (
    IF %ERRORLEVEL% NEQ 0 call %VC_VARS_2019% %APP_ARCH% >nul
    IF %ERRORLEVEL% NEQ 0 GOTO :COMPILE_AND_LINK
)
IF EXIST %VC_VARS_2017% (
    IF %ERRORLEVEL% NEQ 0 call %VC_VARS_2017% %APP_ARCH% >nul
    IF %ERRORLEVEL% NEQ 0 GOTO :COMPILE_AND_LINK
)
GOTO :VS_NOT_FOUND

IF NOT EXIST "C:\Program Files (x86)\Windows Kits\10\Redist\D3D\x64" ( GOTO :KIT_NOT_FOUD )

:COMPILE_AND_LINK
:: Store msvc clutter elsewhere
mkdir msvc_landfill >nul 2>nul
pushd msvc_landfill >nul

:: Compile & Link Options
::------------------------------
:: /TC                  Compile as C code.
:: /TP                  Compile as C++ code.
:: /Oi                  Enable intrinsic functions.
:: /Od 	                Disables optimization.
:: /Qpar                Enable parallel code generation.
:: /Ot                  Favor fast code (over small code).
:: /Ob2                 Enable full inline expansion. [ cfarvin::NOTE ] Debugging impact.
:: /Z7	                Full symbolic debug info. No pdb. (See /Zi, /Zl).
:: /GS	                Detect buffer overruns.
:: /MD	                Multi-thread specific, DLL-specific runtime lib. (See /MDd, /MT, /MTd, /LD, /LDd).
:: /GL	                Whole program optimization.
:: /EHsc                No exception handling (Unwind semantics requrie vstudio env). (See /W1).
:: /I<arg>              Specify include directory.
:: /link                Invoke microsoft linker options.
:: /NXCOMPAT            Comply with Windows Data Execution Prevention.
:: /MACHINE:<arg>       Declare machine arch (should match vcvarsall env setting).
:: /NODEFAULTLIB:<arg>  Ignore a library.
:: /LIBPATH:<arg>       Specify library directory/directories.

:: General Parameters
SET GeneralParameters=/Oi /Qpar /EHsc /GL /nologo /Ot /TP /std:c++latest

:: Debug Paramters
SET DebugParameters=/Od /MTd /W4 /WX /D__DXDEBUG__#1

:: Release Parameters
SET ReleaseParameters=/MT /O2 /W4 /WX /Ob2

:: Include Parameters
SET IncludeParameters=/I%cd%\.. ^
/I%INCLUDE_DIR%

:: Link Parameters
SET LinkParameters=/SUBSYSTEM:CONSOLE ^
/NXCOMPAT ^
/MACHINE:x64 ^
/NODEFAULTLIB:MSVCRTD ^
user32.lib ^
d3d12.lib ^
d3dcompiler.lib ^
dxgi.lib


:: Compiler Invocation
::------------------------------
SET SOURCE_FILES=%SOURCE_DIR%\%APP_NAME%.cpp ^
%SOURCE_DIR%\DxTools.cpp ^
%SOURCE_DIR%\WindowTools.cpp ^
%SOURCE_DIR%\Toolkit.cpp

SET "GeneralInvocation=%SOURCE_FILES% %GeneralParameters% %IncludeParameters% /link %LinkParameters%"

SET "INVOKE_RELEASE=cl %ReleaseParameters% %GeneralInvocation%"
SET "INVOKE_DEBUG=cl %DebugParameters% %GeneralInvocation%"

IF /I "%RELEASE_BUILD%" EQU "1" (echo Release build...) else (echo Debug build...)
IF /I "%RELEASE_BUILD%" EQU "1" (%INVOKE_RELEASE%) else (%INVOKE_DEBUG%)
IF %ERRORLEVEL% NEQ 0 GOTO :exit

:: Copy the application executable to the parent directory
xcopy /y %APP_NAME%.exe %BIN_DIR% >nul

:: Copy necessary dll's to the parent directory.
xcopy /y "C:\Program Files (x86)\Windows Kits\10\Redist\D3D\x64\d3dcompiler_47.dll" %BIN_DIR% >nul

popd >nul
echo Done.
echo.


:: Code format
::------------------------------
SET CLANG_FORMAT_INVOCATION=clang-format -i
%CLANG_FORMAT_INVOCATION% %SOURCE_FILES% %INCLUDE_DIR%/DxTools.h %INCLUDE_DIR%/WindowTools.h %INCLUDE_DIR%/Toolkit.h
GOTO :exit


:VS_NOT_FOUND
echo.
echo Unable to find vcvarsall.bat. Did you install Visual Studio to the default location?
echo This build script requries either Visual Studio 2019 or 2017; with the standard C/C++ toolset.
echo.
GOTO :exit


:KIT_NOT_FOUND
echo.
echo Unable to find the Windows 10 Development Kit: "C:\Program Files (x86)\Windows Kits\10\Redist\D3D\x64"
echo which is requried to build this application.
echo.
GOTO :exit


:exit
