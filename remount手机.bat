@echo off
cls
echo +++++++++++++++++++++++++++++++++++++
echo �򿪿�����ѡ�����USB����
echo �������ֻ�adb
adb wait-for-device
echo �ֻ�verity״̬��
adb disable-verity | find "Successfully disabled verity" 1>nul 2>nul && ( echo verity success & adb reboot & goto reboot )
adb disable-verity | find "verity is already disabled" 1>nul 2>nul && ( echo verity success & adb reboot & goto reboot )
adb disable-verity | find "Device is locked." 1>nul 2>nul && ( goto locked )
adb disable-verity | find "verity cannot be disabled/enabled" 1>nul 2>nul && ( echo USER build )
echo +++++++++++++++++++++++++++++++++++++
echo �������root�İ汾
pause
exit

:reboot
ping -n 5 127.0.0.1 >nul
cls
echo +++++++++++++++++++++++++++++++++++++
echo �������ֻ�adb
adb wait-for-device root
adb remount | find "succeeded" 1>nul 2>nul && ( ping -n 5 127.0.0.1 >nul & goto exit ) || ( echo remount ʧ�� & pause )
goto exit

:locked
cls
echo +++++++++++++++++++++++++++++++++++++
echo �ֻ�δ���������Ƚ����ֻ�
echo ���룺1 �����ֻ� 2 �˳�
set /p keyboard=
if "keyboard" == "1" CALL "����.bat" & goto exit
if "keyboard" == "2" exit

:exit
timeout 2 >nul