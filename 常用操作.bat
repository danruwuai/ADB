@echo off
setlocal enabledelayedexpansion
rem +++++++++++++++++++++++++++++++++++++
rem             手机常用操作         
rem          By zhong @ 2020/03/10
rem
rem            Version ： 1.0
rem +++++++++++++++++++++++++++++++++++++

rem 监测连接状态

:connect
cls
color 07
echo +++++++++++++++++++++++++++++++++++++
echo 查看设备连接状态
for /f %%i in ( 'adb devices ^| find /c "device"' ) do (
    set a=%%i
    if !a! LEQ 1 (
        echo "NO DEVICE CONNECTED" & goto wait-for-device
    )else ( set /a b=!a!-1 & echo Total !b! devices & goto remount )
)

:wait-for-device
cls
color 0c
echo +++++++++++++++++++++++++++++++++++++
echo 手机未连接，请打开USB调试
adb wait-for-device
REM 在通话界面输入*#8011#
::adb shell service call phone 1 s16 "%%2A%%238011%%23"
goto connect

:remount
adb wait-for-device
adb devices > %~dp0\deviceinfo.txt
for /f "skip=1 tokens=1-2 delims=	" %%c in ( deviceinfo.txt ) do (
    if %%d == device echo 连接的手机device %%c
)
del /a/f/q %~dp0\deviceinfo.txt
adb root
adb remount | find "succeeded" 1>nul 2>nul  && ( echo remount success & goto begin )
adb remount | find "Not running as root." 1>nul 2>nul  && ( echo root first & goto root )
adb remount | find "remount failed" 1>nul 2>nul && ( echo remount fail & goto disable-verity )
ping -n 2 127.0.0.1 >nul
goto exit

:root
cls
color 0c
echo +++++++++++++++++++++++++++++++++++++
echo ROOT First
echo 查看手机版本类型
adb shell getprop ro.build.type
echo 查看是否可以关闭verity
adb disable-verity | find "verity cannot be disabled/enabled" 1>nul 2>nul && ( echo USER build ) || ( goto disable-verity )
echo +++++++++++++++++++++++++++++++++++++
echo 请更换可root的版本
pause
goto exit

:disable-verity
cls
color 0c
echo +++++++++++++++++++++++++++++++++++++
echo 手机未remount
echo 手机verity状态
adb disable-verity | find "Device is locked." 1>nul 2>nul && ( goto locked ) || ( echo device unlocked )
adb disable-verity | find "Successfully disabled verity" 1>nul 2>nul && ( adb reboot )
adb disable-verity | find "verity is already disabled" 1>nul 2>nul && ( adb reboot )
adb disable-verity
ping -n 6 127.0.0.1 >nul
cls
goto connect

:locked
cls
color 0c
echo +++++++++++++++++++++++++++++++++++++
echo 手机未解锁，请先解锁手机
echo 输入 ：1 解锁手机  2 退出
set /p keyboard=
if "%keyboard%" == "1" CALL "解锁.bat" & goto connect
if "%keyboard%" == "2" goto exit
goto locked

:begin
cls
color 07
echo +++++++++++++++++++++++++++++++++++++
echo 请根据需要选择操作：
echo 1 启动Camera
echo 2 查看机型信息
echo 3 刷机，进入fastboot
echo 4 开启高通AFlog
echo 5 开启MTK AFlog
echo . 重新检测手机状态
echo 0 退出
echo +++++++++++++++++++++++++++++++++++++
set /p keyboard=
if "%keyboard%" == "0" goto exit
if "%keyboard%" == "1" CALL "启动相机.bat" & goto begin
if "%keyboard%" == "2" CALL "获取机型信息.bat" & goto begin
if "%keyboard%" == "3" CALL "刷机进入fastboot.bat" & goto begin
if "%keyboard%" == "4" CALL "%~dp0\QCM\AF\push_camxoverridesettings.bat" & goto begin
if "%keyboard%" == "5" CALL "%~dp0\MTK\AF\af.bat" & goto begin
if "%keyboard%" == "." goto connect
if "%keyboard%" == "。" goto connect
goto begin

:exit
timeout 2 >nul