@echo off
REM Builds dist\LibraryManager.exe on a Windows PC that has Visual Studio 2022 (C++), CMake and Qt 6.
REM Run it from the "x64 Native Tools Command Prompt for VS 2022" (Start menu), inside this folder.
REM If Qt is not in the default folder:   set QT_DIR=D:\Qt\6.7.3\msvc2022_64   (then run this file)
setlocal
if "%QT_DIR%"=="" set QT_DIR=C:\Qt\6.7.3\msvc2022_64
if not exist "%QT_DIR%\bin\windeployqt.exe" (
  echo Qt was not found in "%QT_DIR%".
  echo Install Qt 6 ^(MSVC 64-bit^) or set QT_DIR to its folder, e.g. set QT_DIR=C:\Qt\6.7.3\msvc2022_64
  exit /b 1
)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QT_DIR%" || goto :fail
cmake --build build || goto :fail
if exist dist rmdir /s /q dist
mkdir dist
copy /y build\LibraryManager.exe dist\ >nul
"%QT_DIR%\bin\windeployqt.exe" --release --no-translations --compiler-runtime dist\LibraryManager.exe || goto :fail
echo.
echo Done. Your program is in the "dist" folder:  dist\LibraryManager.exe
exit /b 0
:fail
echo.
echo BUILD FAILED - copy the messages above and send them to get help.
exit /b 1
