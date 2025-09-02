@echo off
setlocal ENABLEDELAYEDEXPANSION
REM Build Tolk.dll (optional) and NativeTrainer. Auto-detect MSBuild using vswhere if needed.

REM Args: pass "no-tolk" to skip building Tolk, or set BUILD_TOLK=0 in env
set "SKIP_TOLK="
if /i "%~1"=="no-tolk" set "SKIP_TOLK=1"
if not defined SKIP_TOLK if defined BUILD_TOLK if "%BUILD_TOLK%"=="0" set "SKIP_TOLK=1"

set MSBUILD=msbuild
where %MSBUILD% >nul 2>&1
if errorlevel 1 (
  if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set "MSBUILD=%%i"
  )
)
if not exist "%MSBUILD%" (
  if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
)
if not exist "%MSBUILD%" (
  if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
)
if not exist "%MSBUILD%" (
  if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
)
if not exist "%MSBUILD%" (
  echo ERROR: MSBuild not found. Please install Visual Studio Build Tools or run from Developer Command Prompt.
  goto :err
)

echo === Using MSBuild: %MSBUILD% ===

REM Use VS2022 toolset by default (adjust if needed)
set "TOOLSET_OVERRIDE=/p:PlatformToolset=v143"
echo === Using Toolset Override: %TOOLSET_OVERRIDE% ===

set "TOLK_OUT="
set "TOLK_PROJ="
set "TOLK_SRC1=%~dp0tolk-source-code\tolk\src\Tolk.vcxproj"
set "TOLK_SRC2=%~dp0tolk\src\Tolk.vcxproj"
if exist "%TOLK_SRC1%" set "TOLK_PROJ=%TOLK_SRC1%"
if not defined TOLK_PROJ if exist "%TOLK_SRC2%" set "TOLK_PROJ=%TOLK_SRC2%"
if defined SKIP_TOLK goto :tolk_skip
if defined TOLK_PROJ goto :tolk_build
echo WARNING: Tolk source not found. Skipping Tolk build. Speech will work only if tolk.dll is present.
goto :tolk_after

:tolk_build
echo === Building Tolk.dll ^(Release x64^) ===
rem Use a distinct loop var and proper modifier (%%~dpA)
for %%A in ("%TOLK_PROJ%") do set "_tolk_dir=%%~dpA"
pushd "%_tolk_dir%" 1>nul 2>nul
REM Build without forcing toolset; let the project decide
set "_LOG_TOLK=%TEMP%\msbuild_tolk_!RANDOM!.log"
"%MSBUILD%" "Tolk.vcxproj" /m /nologo /v:m /p:Configuration=Release /p:Platform=x64 %TOOLSET_OVERRIDE% > "%_LOG_TOLK%" 2>&1 || (
  echo ERROR: Building Tolk failed. See "%_LOG_TOLK%"
  call :print_tail "%_LOG_TOLK%" 120
  popd & goto :err
)
REM Try to locate tolk.dll under a typical bin folder
set "_found="
for /f "delims=" %%f in ('dir /s /b "bin\\tolk.dll" 2^>nul') do set "_found=%%f"
if not defined _found for /f "delims=" %%f in ('dir /s /b "tolk.dll" 2^>nul') do set "_found=%%f"
if defined _found (
  set "TOLK_OUT=%_found%"
) else (
  echo WARNING: Built Tolk but could not find tolk.dll. Speech will be optional.
)
popd
goto :tolk_after

:tolk_skip
echo NOTE: Skipping Tolk build by request.

:tolk_after

REM If we didn't build Tolk, try to pick an existing tolk.dll
if not defined TOLK_OUT (
  if defined TOLK_DLL if exist "%TOLK_DLL%" set "TOLK_OUT=%TOLK_DLL%"
)
if not defined TOLK_OUT if exist "%~dp0tolk.dll" set "TOLK_OUT=%~dp0tolk.dll"
if not defined TOLK_OUT if exist "%~dp0samples\NativeTrainer\bin\Release\tolk.dll" set "TOLK_OUT=%~dp0samples\NativeTrainer\bin\Release\tolk.dll"
if not defined TOLK_OUT (
  for /f "delims=" %%f in ('dir /s /b "%~dp0tolk.dll" 2^>nul') do (
    if not defined TOLK_OUT set "TOLK_OUT=%%f"
  )
)

echo === Building NativeTrainer ^(Release x64^) ===
pushd "%~dp0samples\NativeTrainer"
REM Don't force PlatformToolset here to avoid toolset mismatches
set "_LOG_NT=%TEMP%\msbuild_nt_!RANDOM!.log"
"%MSBUILD%" NativeTrainer.vcxproj /m /nologo /v:m /p:Configuration=Release /p:Platform=x64 %TOOLSET_OVERRIDE% > "%_LOG_NT%" 2>&1 || (
  echo ERROR: Building NativeTrainer failed. See "%_LOG_NT%"
  call :print_tail "%_LOG_NT%" 120
  goto :err
)
set NT_OUT=%CD%\bin\Release\NativeTrainer.asi
if not exist "%NT_OUT%" (
  echo ERROR: NativeTrainer.asi not found at %NT_OUT%
  goto :err
)
popd

if defined TOLK_OUT goto :copy_tolk_to_bin
echo NOTE: Skipping tolk.dll copy (not built/found).
goto :after_copy_tolk_to_bin
:copy_tolk_to_bin
echo === Copy tolk.dll next to NativeTrainer.asi ===
copy /Y "%TOLK_OUT%" "%~dp0samples\NativeTrainer\bin\Release\tolk.dll" >nul || echo WARNING: Failed to copy tolk.dll
:after_copy_tolk_to_bin

echo === Create/prepare package folder for easy copying to RDR2 directory ===
set "PACK_DIR=%~dp0package_RDR2_A11y"
if not exist "%PACK_DIR%" (
  mkdir "%PACK_DIR%"
  if errorlevel 1 goto :err
)

copy /Y "%NT_OUT%" "%PACK_DIR%\NativeTrainer.asi" >nul
if errorlevel 1 goto :err
if defined TOLK_OUT goto :copy_tolk_to_pkg
echo WARNING: tolk.dll not included in package.
goto :after_copy_tolk_to_pkg
:copy_tolk_to_pkg
copy /Y "%TOLK_OUT%" "%PACK_DIR%\tolk.dll" >nul
if errorlevel 1 echo WARNING: Failed to copy tolk.dll to package
:after_copy_tolk_to_pkg

rem Populate README: prefer repository template if present; otherwise keep existing or create a short stub
if exist "%~dp0README_package.txt" (
  echo Using README template from "%~dp0README_package.txt" to "%PACK_DIR%\README.txt"
  copy /Y "%~dp0README_package.txt" "%PACK_DIR%\README.txt"
) else (
  if not exist "%PACK_DIR%\README.txt" (
    echo Creating minimal README stub in package ^(template not found^)
    echo Copy both files to your Red Dead Redemption 2 game directory ^(where RDR2.exe is^):>"%PACK_DIR%\README.txt"
    echo - NativeTrainer.asi>>"%PACK_DIR%\README.txt"
    echo - tolk.dll ^(optional, needed for NVDA/JAWS speech via Tolk^)>>"%PACK_DIR%\README.txt"
    echo Ensure ScriptHookRDR2 is installed. NVDA should be running for speech.>>"%PACK_DIR%\README.txt"
  ) else (
    echo NOTE: Keeping existing README.txt in package.
  )
)

REM Ensure audio cue wavs are present in package (keep existing ones)
for %%W in (tped.wav tvehicle.wav tprop.wav) do (
  if not exist "%PACK_DIR%\%%W" (
    if exist "%~dp0samples\NativeTrainer\bin\Release\%%W" copy /Y "%~dp0samples\NativeTrainer\bin\Release\%%W" "%PACK_DIR%\%%W" >nul
  )
)

REM Also copy wavs next to built NativeTrainer.asi for local testing
for %%W in (tped.wav tvehicle.wav tprop.wav) do (
  if exist "%PACK_DIR%\%%W" copy /Y "%PACK_DIR%\%%W" "%~dp0samples\NativeTrainer\bin\Release\%%W" >nul
)

echo SUCCESS. Output:
echo   %NT_OUT%
if defined TOLK_OUT echo   %~dp0samples\NativeTrainer\bin\Release\tolk.dll
echo   %PACK_DIR%\NativeTrainer.asi
if defined TOLK_OUT echo   %PACK_DIR%\tolk.dll
echo   %PACK_DIR%\README.txt
exit /b 0

:print_tail
REM Print the last N lines of a file using PowerShell; default to 120 lines
set "_PT_FILE=%~1"
set "_PT_LINES=%~2"
if not defined _PT_LINES set "_PT_LINES=120"
echo --- Last %_PT_LINES% lines of "%_PT_FILE%" ---
powershell -NoProfile -Command "Get-Content -Path '%_PT_FILE:'='''%' -Tail %_PT_LINES% | Out-String"
echo --------------------------------
exit /b 0

:err
echo Build failed. Ensure you are in a Developer Command Prompt for VS (msbuild available).
echo See logs if present matching: "%TEMP%\msbuild_tolk_*.log" and "%TEMP%\msbuild_nt_*.log"
exit /b 1


