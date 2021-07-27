@echo off

echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0
CALL "..\af.bat"

echo 关闭相机
adb shell pkill camera*
timeout 5 >nul
echo +++++++++++++++++++++++++++++++++++++
echo 打开logcat
START CMD /K CALL "..\adb_logcat.bat"
timeout 3 >nul
echo +++++++++++++++++++++++++++++++++++++
echo 打开相机
adb shell am start -a android.media.action.STILL_IMAGE_CAMERA --ei android.intent.extras.Camera_FACING 0
timeout 10 >nul
adb shell input keyevent 27
echo +++++++++++++++++++++++++++++++++++++
echo 切换到VIDEO并录制
pause
timeout 5 >nul
