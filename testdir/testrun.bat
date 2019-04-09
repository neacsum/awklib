@echo off
rem
rem Run one test.
rem

set AWK="..\x86\debug\awk.exe"

%awk% --version
.\echo -n "Running test %~n1... "
maketest %1

%awk% -f %~n1.awk %~n1.in >%~n1.out

fc /A %~n1.ref %~n1.out >NUL
if errorlevel 1 (
  .\echo failed
  fc /A %~n1.ref %~n1.out
) else (
  del %~n1.awk %~n1.in %~n1.ref %~n1.out
  .\echo ok
)

