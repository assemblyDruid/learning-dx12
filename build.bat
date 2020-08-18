@echo off

::------------------------------
::
:: Relase / Debug
::
::------------------------------
SET /A ReleaseBuild=0


::------------------------------
::
:: Environment Settings
:: App name, architecture
::
::------------------------------
SET ScriptDirectory=%cd%
SET SourceDir=%ScriptDirectory%\src
SET IncludeDirectory=%ScriptDirectory%\include
SET BinaryDirectory=%ScriptDirectory%\bin\
SET ApplicationName=dx12
SET ApplicationArch=x64


::------------------------------
::
:: Compile Engine
:: Requires Visual Studio 2019
::
::------------------------------
SET VisualStudio2019Community="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
SET VisualStudio2019Professional="C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat"

IF EXIST %VisualStudio2019Professional% (
    call %VisualStudio2019Professional% %ApplicationArch% >nul
    GOTO :COMPILE_AND_LINK
)
IF EXIST %VisualStudio2019Community% (
    call %VisualStudio2019Community% %ApplicationArch% >nul
    GOTO :COMPILE_AND_LINK
)
GOTO :VS_NOT_FOUND


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
:: /MP                  Build w/ multiple processes.
:: /Zc:inline           Emit less symbol info for inline functions.
:: /Zc:wchar_t          Treat wide characters as a default (internal) type.

:: Compiler Invocation
::------------------------------
SET GeneralParameters=/Oi /Qpar /EHsc /GL /nologo /Ot /TP /MP8 ^
/std:c++latest /Zc:wchar_t /D"_UNICODE" /D"UNICODE" /Zc:inline
SET DebugParameters=/Od /MTd /W4 /WX /D__DXDEBUG__#1
SET ReleaseParameters=/MT /O2 /W4 /WX /Ob2
SET IncludeParameters=/I%IncludeDirectory%

SET LinkParameters=/SUBSYSTEM:CONSOLE ^
/NXCOMPAT ^
/MACHINE:x64 ^
/NODEFAULTLIB:MSVCRTD ^
user32.lib ^
d3d12.lib ^
d3dcompiler.lib ^
dxgi.lib

SET SourceFiles=%SourceDir%\%ApplicationName%.cpp ^
%SourceDir%\Dx.cpp ^
%SourceDir%\WindowTools.cpp ^
%SourceDir%\Toolkit.cpp ^
%SourceDir%\Controller.cpp

:: Format source files and headers; wait until this process finishes to compile.
echo Formatting files...
SET ClangFormatInvocation=clang-format -i
START /b /wait CMD /C %ClangFormatInvocation% %SourceFiles% %IncludeDirectory%/Dx.h ^
%IncludeDirectory%/WindowTools.h %IncludeDirectory%/Toolkit.h %IncludeDirectory%/Controller.h
ECHO Done.
echo.

SET "GeneralInvocation=%SourceFiles% %GeneralParameters% %IncludeParameters% /link %LinkParameters%"

SET "InvokeRelease=cl %ReleaseParameters% %GeneralInvocation%"
SET "InvokeDebug=cl %DebugParameters% %GeneralInvocation%"

IF /I "%ReleaseBuild%" EQU "1" (echo Release build...) else (echo Debug build...)
IF /I "%ReleaseBuild%" EQU "1" (%InvokeRelease%) else (%InvokeDebug%)
IF %ERRORLEVEL% NEQ 0 GOTO :exit

:: Copy the application executable to the parent directory
xcopy /y %ApplicationName%.exe %BinaryDirectory% >nul

:: Copy necessary dll's to the parent directory.
SET WindowsDevKitDirectory="C:\Program Files (x86)\Windows Kits\10\Redist\D3D\x64\"
IF NOT EXIST %WindowsDevKitDirectory% GOTO :KIT_NOT_FOUD
xcopy /y "C:\Program Files (x86)\Windows Kits\10\Redist\D3D\x64\d3dcompiler_47.dll" %BinaryDirectory% >nul
popd >nul
echo Done.
echo.
GOTO :exit


:VS_NOT_FOUND
echo.
echo Unable to find vcvarsall.bat.
echo Visual Studio 2019 must be installed in the default location.
echo.
GOTO :exit


:KIT_NOT_FOUD
echo.
echo Unable to find the Windows 10 Development Kit at the following location:
echo %WindowsDevKitDirectory%
echo.
GOTO :exit


:exit
