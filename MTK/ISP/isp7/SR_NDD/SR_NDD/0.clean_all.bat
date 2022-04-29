adb root
adb shell rm -rf /data/vendor/camera_dump
adb shell mkdir /data/vendor/camera_dump -p

adb shell rm -rf /data/vendor/VHDR_Data
adb shell mkdir /data/vendor/VHDR_Data  -p

adb shell rm /sdcard/DCIM/Camera/*
adb shell rm /sdcard/DCIM/Screenshots/*
adb shell rm /sdcard/DCIM/Camera/mfsr/*

adb shell rm /storage/emulated/0/CameraDebug -rf
adb logcat -c 
adb logcat -G 256M

pause


