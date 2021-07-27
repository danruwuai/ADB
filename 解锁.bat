@echo off
color 0c
cls
echo +++++++++++++++++++++++++++++++++++++
echo 打开开发者选项，开启USB调试
echo 打开OEM解锁开关
pause

:locked
cls
echo +++++++++++++++++++++++++++++++++++++
echo 连接上手机adb
adb wait-for-device
echo 选择解锁选项，若没有对应，请检查设备是否支持
adb reboot bootloader
fastboot flashing unlock
ping -n 20 127.0.0.1>nul
cls
echo +++++++++++++++++++++++++++++++++++++
echo 连接上手机adb
adb wait-for-device
echo 手机解锁状态 ：
adb shell getprop ro.boot.vbmeta.device_state
adb shell getprop ro.boot.vbmeta.device_state | find "unlocked" 1>nul 2>nul && ( CALL "remount手机.bat" & goto exit ) || ( goto lock )
goto exit

:lock
cls
echo +++++++++++++++++++++++++++++++++++++
echo 手机解锁失败，请检查具体原因
echo 输入 ： 1 重新解锁手机 2 退出
set /p keyboard= 
if "%keyboard%" == "1" goto locked
if "%keyboard%" == "2" exit
goto lock

:exit
timeout 2 >nul