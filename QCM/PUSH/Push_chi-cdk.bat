@echo off
cls
echo +++++++++++++++++++++++++++++++++++++
REM PUSH CHI_CDK

:begin
echo 输入sensor的名称，push对应的参数bin文件：0--退出
set /p sensor=
if "%sensor%" == "846" goto after
if "%sensor%" == "0" goto exit
if "%sensor%" == "" goto begin
goto begin

:after
echo 选择push不同的bin文件 ：1--tuned 2--sensormodule 0--退出
set /p choose=
if "%choose%" == "1" goto push_tuned
if "%choose%" == "2" goto push_sensormodule
if "%choose%" == "0" goto exit
if "%choose%" == "" goto after
goto after

:push_tuned
echo Copy %sensor%.bin
copy X:\xxx\andorid\out\target\product\kona\vendor\lib64\camera\com.qti.tuned.*%sensor%*.bin %~dp0
REM 获取对应sensor名的完整bin名称。
for /f %%a in ('dir /o-d /tc /b com.qti.tunned.*%sensor%*.bin') do (
	echo 文件完整信息：%%a
	set filename=%%~na%%~xa
	)
timeout 2 >nul

echo 连接手机adb
adb wait-for-device root
adb wait-for-device remount
adb shell umount -nlf /vendor/etc
adb shell umount -nlf /vendor/lib64
adb shell umount -nlf /vendor/lib
echo 查看手机中的%filename%
echo +++++++++++++++++++++++++++++++++++++
adb shell ls -l vendor/lib64/camera/%filename%
echo +++++++++++++++++++++++++++++++++++++
echo off
echo off
timeout 2 >nul

echo Push %filename%
adb push %filename% /vendor/lib64/camera/

echo 查看手机中的%filename%
echo +++++++++++++++++++++++++++++++++++++
adb shell ls -l vendor/lib64/camera/%filename%
echo +++++++++++++++++++++++++++++++++++++
pause
echo Delete %filename%
del %filename%
timeout 2 >nul
adb shell killall -9 android.hardware.camera.provider@2.4-service_64
goto exit

:push_sensormodule
echo Copy %sensor%.bin
copy X:\xxx\andorid\out\target\product\kona\vendor\lib64\camera\com.qti.sensormodule*%sensor%*.bin %~dp0
REM 获取对应sensor名的完整bin名称。
for /f %%a in ('dir /o-d /tc /b com.qti.sensormodule*%sensor%*.bin') do (
	echo 文件完整信息：%%a
	set filename=%%~na%%~xa
	)
timeout 2 >nul

echo 连接手机adb
adb wait-for-device root
adb wait-for-device remount
adb shell umount -nlf /vendor/etc
adb shell umount -nlf /vendor/lib64
adb shell umount -nlf /vendor/lib
echo 查看手机中的%filename%
echo +++++++++++++++++++++++++++++++++++++
adb shell ls -l vendor/lib64/camera/%filename%
echo +++++++++++++++++++++++++++++++++++++
echo off
echo off
timeout 2 >nul

echo Push %filename%
adb push %filename% /vendor/lib64/camera/

echo 查看手机中的%filename%
echo +++++++++++++++++++++++++++++++++++++
adb shell ls -l vendor/lib64/camera/%filename%
echo +++++++++++++++++++++++++++++++++++++
pause
echo Delete %filename%
del %filename%
timeout 2 >nul
adb shell killall -9 android.hardware.camera.provider@2.4-service_64
goto exit

:exit
echo 退出
timeout 2 >nul