@echo off

echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0
echo ����AF_log����
CALL "%~dp0\af_log.bat"
timeout 4
echo +++++++++++++++++++++++++++++++++++++
echo �������
adb shell am start -a android.media.action.STILL_IMAGE_CAMERA --ei android.intent.extras.Camera_FACING 0

echo +++++++++++++++++++++++++++++++++++++
echo ��MTK logger����¼log��Ϣ
echo �����������밴������
pause
echo +++++++++++++++++++++++++++++++++++++
echo ����Inf��OTP����
set /p Inf_OTP=
echo +++++++++++++++++++++++++++++++++++++
echo ����Mac��OTP����
set /p Mac_OTP=
set /a Mac=%Mac_OTP%+8
echo +++++++++++++++++++++++++++++++++++++
echo ��ʼ�ֶ�����Position
adb shell setprop vendor.debug.af_motor.disable 1
rem Postion��Ӧλ��
set /a step=(Mac-Inf_OTP)/9
for /l %%p in (%Inf_OTP%,%step%,%Mac%) do (
::set a=%%p 
echo +++++++++++++++++++++++++++++++++++++
echo �ƶ�Postion��%%p
adb shell setprop vendor.debug.af_motor.position %%p
timeout 3 >nul 
)
adb shell setprop vendor.debug.af_motor.disable 0
timeout 4 >nul