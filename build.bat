@echo off
REM Builds FeatureLens (GUI) + FeatureLensBatch (headless) and optionally generates all result images.
REM Usage:  build.bat            -> build only
REM         build.bat results    -> build, then run tools\run_all.py (needs Python + Pillow)
REM Edit CMakePresets.json (CMAKE_PREFIX_PATH) if your Qt is installed somewhere else.
setlocal
cd /d "%~dp0"
set "PATH=C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;C:\Qt\6.9.1\mingw_64\bin;%PATH%"
cmake --fresh --preset windows-msvc || goto :err
cmake --build build --config Release || goto :err
echo.
echo Built: build\Release\FeatureLens.exe  and  build\Release\FeatureLensBatch.exe
if /i "%1"=="results" (
    python tools\run_all.py --exe build\Release\FeatureLensBatch.exe || goto :err
)
exit /b 0
:err
echo BUILD FAILED
exit /b 1
