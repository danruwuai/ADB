@echo off
cls
echo +++++++++++++++++++++++++++++++++++++
REM PUSH CAMX-LIB-STATS
echo Copy com.qti.stats.af.so
copy X:\xxx\andorid\out\target\product\kona\vendor\lib64\camera\components\com.qti.stats.af.so %~dp0
REM %~dp0显示的是当前bat目录的位置
timeout 2 >nul

echo 连接手机adb
adb wait-for-device root
adb wait-for-device remount
adb shell umount -nlf /vendor/etc
adb shell umount -nlf /vendor/lib64
adb shell umount -nlf /vendor/lib
echo 查看手机中的com.qti.stats.af.so
echo +++++++++++++++++++++++++++++++++++++
adb shell ls -l vendor/lib64/camera/components/com.qti.stats.af.so
echo +++++++++++++++++++++++++++++++++++++
echo off
echo off
timeout 2 >nul

echo Push com.qti.stats.af.so
adb push com.qti.stats.af.so /vendor/lib64/camera/components/com

echo 查看手机中的com.qti.stats.af.so
echo +++++++++++++++++++++++++++++++++++++
adb shell ls -l vendor/lib64/camera/components/com.qti.stats.af.so
echo +++++++++++++++++++++++++++++++++++++
pause
echo Delete com.qti.stats.af.so
del com.qti.stats.af.so
timeout 2 >nul
adb shell killall -9 android.hardware.camera.provider@2.4-service_64