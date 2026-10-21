@echo off
rem Build the screencap branch: apply patches, configure with the `default` preset, build + install.
rem Produces build\default\install\bin\Audacity4.exe in this worktree.
rem Needs CMakeUserPresets.json (copied from the main checkout; it is gitignored).
setlocal
cd /d "%~dp0.."
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
sh screencap/apply-patches.sh || exit /b 1
cmake --preset default || exit /b 1
cmake --build --preset install || exit /b 1
echo built: %CD%\build\default\install\bin\Audacity4.exe
