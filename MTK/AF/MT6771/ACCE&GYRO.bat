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
echo ��logcat
START CMD /K CALL "..\adb_logcat.bat"
timeout 3 >nul
echo +++++++++++++++++++++++++++++++++++++
echo �����
adb shell am start -a android.media.action.STILL_IMAGE_CAMERA --ei android.intent.extras.Camera_FACING 0
timeout 10 >nul
adb shell input keyevent 27
echo +++++++++++++++++++++++++++++++++++++
echo �л���VIDEO��¼��
pause
timeout 5 >nul
