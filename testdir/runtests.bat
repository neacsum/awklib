@echo off
rem
rem Run all tests in current directory.
rem


for %%i in (tests\*.tst) do (
  .\echo -n "Running test %%~ni... "
  maketest %%i
  awk -f %%~ni.awk %%~ni.in >%%~ni.out
  fc /A %%~ni.ref %%~ni.out >NUL
  if errorlevel 1 (
    .\echo failed
    fc /A %%~ni.ref %%~ni.out
  ) else (
    del %%~ni.awk %%~ni.in %%~ni.ref %%~ni.out
    .\echo ok
  )
)

