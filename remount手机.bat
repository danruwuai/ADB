@echo off
cls
echo +++++++++++++++++++++++++++++++++++++
echo 打开开发者选项，开启USB调试
echo 连接上手机adb
adb wait-for-device
echo 手机verity状态：
adb disable-verity | find "Successfully disabled verity" 1>nul 2>nul && ( echo verity success & adb reboot & goto reboot )
adb disable-verity | find "verity is already disabled" 1>nul 2>nul && ( echo verity success & adb reboot & goto reboot )
adb disable-verity | find "Device is locked." 1>nul 2>nul && ( goto locked )
adb disable-verity | find "verity cannot be disabled/enabled" 1>nul 2>nul && ( echo USER build )
echo +++++++++++++++++++++++++++++++++++++
echo 请更换可root的版本
pause
exit

:reboot
ping -n 5 127.0.0.1 >nul
cls
echo +++++++++++++++++++++++++++++++++++++
echo 连接上手机adb
adb wait-for-device root
adb remount | find "succeeded" 1>nul 2>nul && ( ping -n 5 127.0.0.1 >nul & goto exit ) || ( echo remount 失败 & pause )
goto exit

:locked
cls
echo +++++++++++++++++++++++++++++++++++++
echo 手机未解锁，请先解锁手机
echo 输入：1 解锁手机 2 退出
set /p keyboard=
if "keyboard" == "1" CALL "解锁.bat" & goto exit
if "keyboard" == "2" exit

:exit
timeout 2 >nul