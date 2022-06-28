@echo off
set workdir=%~dp0
echo %workdir%
for /r %workdir% %%i in (.) do (
pushd %%i
if exist .\Binaries\ rd /s /q Binaries
if exist .\Intermediate\ rd /s /q Intermediate
if exist .\Saved\ rd /s /q Saved
if exist .\.vs\ rd /s /q .vs
)