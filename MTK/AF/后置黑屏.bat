adb wait-for-device root
adb wair-for-device remount
adb shell setenforce 0
adb shell setprop vendor.debug.camera.remo.enable 0
adb shell pkill camera*