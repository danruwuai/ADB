@echo off
color 0c
cls
echo +++++++++++++++++++++++++++++++++++++
echo �򿪿�����ѡ�����USB����
echo ��OEM��������
pause

:locked
cls
echo +++++++++++++++++++++++++++++++++++++
echo �������ֻ�adb
adb wait-for-device
echo ѡ�����ѡ���û�ж�Ӧ�������豸�Ƿ�֧��
adb reboot bootloader
fastboot flashing unlock
ping -n 20 127.0.0.1>nul
cls
echo +++++++++++++++++++++++++++++++++++++
echo �������ֻ�adb
adb wait-for-device
echo �ֻ�����״̬ ��
adb shell getprop ro.boot.vbmeta.device_state
adb shell getprop ro.boot.vbmeta.device_state | find "unlocked" 1>nul 2>nul && ( CALL "remount�ֻ�.bat" & goto exit ) || ( goto lock )
goto exit

:lock
cls
echo +++++++++++++++++++++++++++++++++++++
echo �ֻ�����ʧ�ܣ��������ԭ��
echo ���� �� 1 ���½����ֻ� 2 �˳�
set /p keyboard= 
if "%keyboard%" == "1" goto locked
if "%keyboard%" == "2" exit
goto lock

:exit
timeout 2 >nul