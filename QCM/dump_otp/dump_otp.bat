adb wait-for-device root
adb remount
adb shell setprop vendor.debug.camera.dumpSensorEEPROMData TRUE
adb shell killall -9 android.hardware.camera.provider@2.4-service_64
pause