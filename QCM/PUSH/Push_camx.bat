@echo off
cls
echo +++++++++++++++++++++++++++++++++++++
REM PUSH CAMX
echo Copy camera.qcom.so
copy X:\xxx\andorid\out\target\product\kona\vendor\lib64\hw\camera.qcom.so %~dp0
REM %~dp0显示的是当前bat目录的位置
timeout 2 >nul

echo 连接手机adb
adb wait-for-device root
adb wait-for-device remount
adb shell umount -nlf /vendor/etc
adb shell umount -nlf /vendor/lib64
adb shell umount -nlf /vendor/lib
echo 查看手机中的camera.qcom.so
echo +++++++++++++++++++++++++++++++++++++
adb shell ls -l vendor/lib64/hw/camera.qcom.so
echo +++++++++++++++++++++++++++++++++++++
echo off
echo off
timeout 2 >nul

echo Push camera.qcom.so
adb push camera.qcom.so /vendor/lib64/hw/

echo 查看手机中的camera.qcom.so
echo +++++++++++++++++++++++++++++++++++++
adb shell ls -l vendor/lib64/hw/camera.qcom.so
echo +++++++++++++++++++++++++++++++++++++
pause
echo Delete camera.qcom.so
del camera.qcom.so
timeout 2 >nul
adb shell killall -9 android.hardware.camera.provider@2.4-service_64