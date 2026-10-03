@echo off
setlocal
if /I "%~1"=="--preview" goto preview
powershell.exe -NoLogo -NoProfile -File "%~dp0tools\clean-workspace.ps1"
set "cleanupExit=%errorlevel%"
echo.
if not "%cleanupExit%"=="0" echo Cleanup stopped with errors. See the output above.
pause
exit /b %cleanupExit%
:preview
powershell.exe -NoLogo -NoProfile -File "%~dp0tools\clean-workspace.ps1" -Preview
exit /b %errorlevel%
