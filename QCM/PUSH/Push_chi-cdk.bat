@echo off
cls
echo +++++++++++++++++++++++++++++++++++++
REM PUSH CHI_CDK

:begin
echo ����sensor�����ƣ�push��Ӧ�Ĳ���bin�ļ���0--�˳�
set /p sensor=
if "%sensor%" == "846" goto after
if "%sensor%" == "0" goto exit
if "%sensor%" == "" goto begin
goto begin

:after
echo ѡ��push��ͬ��bin�ļ� ��1--tuned 2--sensormodule 0--�˳�
set /p choose=
if "%choose%" == "1" goto push_tuned
if "%choose%" == "2" goto push_sensormodule
if "%choose%" == "0" goto exit
if "%choose%" == "" goto after
goto after

:push_tuned
echo Copy %sensor%.bin
copy X:\xxx\andorid\out\target\product\kona\vendor\lib64\camera\com.qti.tuned.*%sensor%*.bin %~dp0
REM ��ȡ��Ӧsensor��������bin���ơ�
for /f %%a in ('dir /o-d /tc /b com.qti.tunned.*%sensor%*.bin') do (
	echo �ļ�������Ϣ��%%a
	set filename=%%~na%%~xa
	)
timeout 2 >nul

echo �����ֻ�adb
adb wait-for-device root
adb wait-for-device remount
adb shell umount -nlf /vendor/etc
adb shell umount -nlf /vendor/lib64
adb shell umount -nlf /vendor/lib
echo �鿴�ֻ��е�%filename%
echo +++++++++++++++++++++++++++++++++++++
adb shell ls -l vendor/lib64/camera/%filename%
echo +++++++++++++++++++++++++++++++++++++
echo off
echo off
timeout 2 >nul

echo Push %filename%
adb push %filename% /vendor/lib64/camera/

echo �鿴�ֻ��е�%filename%
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
REM ��ȡ��Ӧsensor��������bin���ơ�
for /f %%a in ('dir /o-d /tc /b com.qti.sensormodule*%sensor%*.bin') do (
	echo �ļ�������Ϣ��%%a
	set filename=%%~na%%~xa
	)
timeout 2 >nul

echo �����ֻ�adb
adb wait-for-device root
adb wait-for-device remount
adb shell umount -nlf /vendor/etc
adb shell umount -nlf /vendor/lib64
adb shell umount -nlf /vendor/lib
echo �鿴�ֻ��е�%filename%
echo +++++++++++++++++++++++++++++++++++++
adb shell ls -l vendor/lib64/camera/%filename%
echo +++++++++++++++++++++++++++++++++++++
echo off
echo off
timeout 2 >nul

echo Push %filename%
adb push %filename% /vendor/lib64/camera/

echo �鿴�ֻ��е�%filename%
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
echo �˳�
timeout 2 >nul