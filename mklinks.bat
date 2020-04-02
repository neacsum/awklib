@echo off
rem
rem  Create symbolic links for included projects and output directories.
rem

if defined DEV_ROOT goto MAKELINKS
echo Environment variable DEV_ROOT is not set!
ECHO Cannot create symlinks.
goto :EOF

:MAKELINKS
if not exist include\nul mkdir include
cd include
if not exist utpp\nul mklink /d utpp %DEV_ROOT%\utpp\include\utpp

cd ..
if not exist lib\nul mklink /d lib %DEV_ROOT%\lib
