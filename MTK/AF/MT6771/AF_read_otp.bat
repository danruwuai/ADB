@echo off

echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0
CALL "..\af.bat"

echo �ر����
adb shell pkill camera*
timeout 5 >nul
echo +++++++++++++++++++++++++++++++++++++
echo ��ȡOTP
START CMD /K CALL "Read_otp.bat"
timeout 3 >nul
adb shell am start -a android.media.action.STILL_IMAGE_CAMERA --ei android.intent.extras.Camera_FACING 0
timeout 2 >nul