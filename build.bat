@echo off
setlocal EnableDelayedExpansion

pushd %~dp0

set outputname=raytracer
set outputexe="%outputname%.exe"

echo.

set defines=/DNOMINMAX /DUNICODE /D_UNICODE /D_CRT_SECURE_NO_WARNINGS
set compiler_flags=/nologo /std:c++20 /FC /Zc:preprocessor /EHsc /GR- /WX /W4 /w44062 /wd4201 /wd4324
set linker_flags=/nologo /INCREMENTAL:NO /SUBSYSTEM:CONSOLE
set libs=kernel32.lib user32.lib

set translation_units= ^
..\src\commands.cpp ^
..\src\main.cpp ^
..\src\output_win32.cpp ^
..\src\raytracer.cpp ^
..\src\shapes.cpp ^
..\src\software_raytracer.cpp

set "should_build_release="

for %%x in (%*) do (
    if "%%~x" == "--release" (
        set "should_build_release=true"
        goto :set_flags
    )
)

:set_flags
if defined should_build_release (
   echo     Compiling RELEASE build...
   set defines=%defines% /DNDEBUG /DAE_RELEASE
   set compiler_flags=%compiler_flags% /O2 /Oi /MT
) else (
   echo     Compiling DEBUG build...
   set defines=%defines% /DDEBUG /D_DEBUG /DAE_DEBUG
   set compiler_flags=%compiler_flags% /Od /MTd /Zi
   set linker_flags=%linker_flags% /DEBUG:FULL /PDB:"%outputname%.pdb"
)

echo.

if not exist bin mkdir bin

pushd .\bin

cl %compiler_flags% ^
    %defines% ^
    /I"%cd%\src" ^
    %translation_units% ^
    /Fe: %outputexe% ^
    /link %linker_flags% ^
    %libs%

if %errorlevel% == 0 (
    copy /b/y %outputexe% ..\%outputexe%
)

popd
popd

echo.
endlocal
