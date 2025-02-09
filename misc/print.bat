@echo off
REM example for the print command

REM form feed
echo >> %1

copy %1 LPT1:
