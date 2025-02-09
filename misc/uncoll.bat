@echo off
REM example of usage of bmuncoll
REM
REM usage: uncoll full_path_to_collection_file

REM existing temporary working directory
REM set TEMP=C:\TEMP

REM external program to call
set EXTPGM=dir /b

if "%2" == "execute" goto EXEC

cd %TEMP%
mkdir bmuncoll
cd bmuncoll
bmuncoll -x %1

if errorlevel 1 goto ERROR

for %%f in (*.* *) do call uncoll.bat %%f execute
goto DONE

:EXEC
echo.
echo Processing %1...
%EXTPGM% %1
if errorlevel 1 goto EXECERR
echo ok.
del %1
goto END

:EXECERR
echo failed!
goto END

:ERROR
echo.
echo *** ERROR ***
goto DONE

:DONE
cd ..
rmdir bmuncoll
goto END

:END
set EXTPGM=
