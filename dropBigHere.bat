@echo off
cd /d "%~dp0"
bigDecrypter.exe %1 "%~d1%~p1%~n1_decrypted%~x1"
pause