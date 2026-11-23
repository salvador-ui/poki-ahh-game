@echo off
rem Builds SquadRush.g3a with the PrizmSDK. Usage: make.bat   (or: make.bat clean)
if "%FXCGSDK%"=="" set FXCGSDK=C:/Users/salvador/PrizmSDK/PrizmSDK-win-0.6
"%FXCGSDK%\bin\make.exe" %*
