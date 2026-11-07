@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 (
  echo Open an x64 Native Tools Command Prompt for Visual Studio first.
  exit /b 1
)
pushd "%~dp0"
if not exist build mkdir build
rc /nologo /fo build\app.res app.rc
if errorlevel 1 goto failed
cl /nologo /std:c++20 /EHsc /W4 /WX /permissive- /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN src\main.cpp /Fo:build\main.obj /Fe:build\veyra.exe /link build\app.res /SUBSYSTEM:WINDOWS user32.lib gdi32.lib bcrypt.lib dwmapi.lib gdiplus.lib
if errorlevel 1 goto failed
cl /nologo /std:c++20 /EHsc /W4 /WX tests\core_tests.cpp /Fo:build\core.obj /Fe:build\core-tests.exe
if errorlevel 1 goto failed
build\core-tests.exe
if errorlevel 1 goto failed
cl /nologo /std:c++20 /EHsc /W4 /WX /permissive- /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN /DFOUNDATION_PATCH_TESTS tests\local_pass_fixture.cpp /Fo:build\fixture.obj /Fe:build\local-pass-fixture.exe bcrypt.lib
if errorlevel 1 goto failed
build\local-pass-fixture.exe
if errorlevel 1 goto failed
popd
exit /b 0
:failed
popd
exit /b 1
