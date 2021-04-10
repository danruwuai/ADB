@echo off
setlocal enabledelayedexpansion
rem +++++++++++++++++++++++++++++++++++++
rem             �ֻ����ò���         
rem          By zhong @ 2020/03/10
rem
rem            Version �� 1.0
rem +++++++++++++++++++++++++++++++++++++

rem �������״̬

:connect
cls
color 07
echo +++++++++++++++++++++++++++++++++++++
echo �鿴�豸����״̬
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
echo �ֻ�δ���ӣ����USB����
adb wait-for-device
REM ��ͨ����������*#8011#
::adb shell service call phone 1 s16 "%%2A%%238011%%23"
goto connect

:remount
adb wait-for-device
adb devices > %~dp0\deviceinfo.txt
for /f "skip=1 tokens=1-2 delims=	" %%c in ( deviceinfo.txt ) do (
    if %%d == device echo ���ӵ��ֻ�device %%c
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
echo �鿴�ֻ��汾����
adb shell getprop ro.build.type
echo �鿴�Ƿ���Թر�verity
adb disable-verity | find "verity cannot be disabled/enabled" 1>nul 2>nul && ( echo USER build ) || ( goto disable-verity )
echo +++++++++++++++++++++++++++++++++++++
echo �������root�İ汾
pause
goto exit

:disable-verity
cls
color 0c
echo +++++++++++++++++++++++++++++++++++++
echo �ֻ�δremount
echo �ֻ�verity״̬
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
echo �ֻ�δ���������Ƚ����ֻ�
echo ���� ��1 �����ֻ�  2 �˳�
set /p keyboard=
if "%keyboard%" == "1" CALL "����.bat" & goto connect
if "%keyboard%" == "2" goto exit
goto locked

:begin
cls
color 07
echo +++++++++++++++++++++++++++++++++++++
echo �������Ҫѡ�������
echo 1 ����Camera
echo 2 �鿴������Ϣ
echo 3 ˢ��������fastboot
echo 4 ������ͨAFlog
echo 5 ����MTK AFlog
echo . ���¼���ֻ�״̬
echo 0 �˳�
echo +++++++++++++++++++++++++++++++++++++
set /p keyboard=
if "%keyboard%" == "0" goto exit
if "%keyboard%" == "1" CALL "�������.bat" & goto begin
if "%keyboard%" == "2" CALL "��ȡ������Ϣ.bat" & goto begin
if "%keyboard%" == "3" CALL "ˢ������fastboot.bat" & goto begin
if "%keyboard%" == "4" CALL "%~dp0\QCM\AF\push_camxoverridesettings.bat" & goto begin
if "%keyboard%" == "5" CALL "%~dp0\MTK\AF\af.bat" & goto begin
if "%keyboard%" == "." goto connect
if "%keyboard%" == "��" goto connect
goto begin

:exit
timeout 2 >nul