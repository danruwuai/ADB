@echo off

echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0

echo +++++++++++++++++++++++++++++++++++++
echo 关闭相机
adb shell pkill camera*
timeout 4
echo +++++++++++++++++++++++++++++++++++++
START CMD /K CALL "logcat_initaf.bat"
timeout 4
echo 开启相机
adb shell am start -a android.media.action.STILL_IMAGE_CAMERA --ei android.intent.extras.Camera_FACING 0
timeout 4
