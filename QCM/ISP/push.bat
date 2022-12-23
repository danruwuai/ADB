adb root
adb remount
adb push  camxoverridesettings.txt vendor/etc/camera
::adb reboot
adb shell killall -9 android.hardware.camera.provider@2.4-service_64