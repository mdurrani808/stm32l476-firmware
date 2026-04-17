@echo off
setlocal

if "%~1"=="" (
  echo Usage: run_me_on_windows_for_dbc_to_c.bat path\to\file.dbc
  exit /b 2
)

python "%~dp0dbc_to_c.py" "%~1" "%~dp0..\App\dbc\can_dbc_text.c" --install-dbc "%~dp0..\App\dbc\file.dbc"
