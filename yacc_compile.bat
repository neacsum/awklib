@echo off
cd src
del /q ytab.h
yacc -d -o ytab.cpp awkgram.y
REM Different versions of YACC give different naems to .H file
REM It can be y.tab.h or ytab.h
if exist ytab.h goto :EOF
ren ytab.tab.h ytab.h
