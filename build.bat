@echo off
REM Simple launcher - just double-click this
powershell -ExecutionPolicy Bypass -File "%~dp0build.ps1" %*
pause
