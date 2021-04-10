adb root
adb remount
adb shell pkill camera*
timeout 5
adb shell am start -a android.media.action.STILL_IMAGE_CAMERA --ei android.intent.extras.Camera_FACING 0
timeout 5
adb shell setprop vendor.debug.ae_mgr.enable 1
adb shell setprop vendor.debug.ae_mgr.lock 1
adb shell setprop vendor.debug.ae_mgr.preview.update 1
adb shell setprop vendor.debug.ae_mgr_ccu.enable 0

::100
echo ISO 100
adb shell setprop vendor.debug.ae_mgr.shutter 30001
adb shell setprop vendor.debug.ae_mgr.sensorgain 2048
adb shell setprop vendor.debug.ae_mgr.ispgain 1024

adb shell setprop vendor.debug.ae_mgr.preview.update 1

pause
adb shell input keyevent 27

::200
::echo ISO 200
::adb shell setprop vendor.debug.ae_mgr.shutter 15000
::adb shell setprop vendor.debug.ae_mgr.sensorgain 4096
::adb shell setprop vendor.debug.ae_mgr.ispgain 1024

::adb shell setprop vendor.debug.ae_mgr.preview.update 1

::pause
::adb shell input keyevent 27

::400
echo ISO 400
adb shell setprop vendor.debug.ae_mgr.shutter 7500
adb shell setprop vendor.debug.ae_mgr.sensorgain 8192
adb shell setprop vendor.debug.ae_mgr.ispgain 1024

adb shell setprop vendor.debug.ae_mgr.preview.update 1

pause
adb shell input keyevent 27

::800
echo ISO 800
adb shell setprop vendor.debug.ae_mgr.shutter 3750
adb shell setprop vendor.debug.ae_mgr.sensorgain 15872
adb shell setprop vendor.debug.ae_mgr.ispgain 1058

adb shell setprop vendor.debug.ae_mgr.preview.update 1

pause
adb shell input keyevent 27

::1600
echo ISO 1600
adb shell setprop vendor.debug.ae_mgr.shutter 1875
adb shell setprop vendor.debug.ae_mgr.sensorgain 15872
adb shell setprop vendor.debug.ae_mgr.ispgain 2114

adb shell setprop vendor.debug.ae_mgr.preview.update 1

pause
adb shell input keyevent 27

::3200
echo ISO 3200
adb shell setprop vendor.debug.ae_mgr.shutter 938
adb shell setprop vendor.debug.ae_mgr.sensorgain 15872
adb shell setprop vendor.debug.ae_mgr.ispgain 4228

adb shell setprop vendor.debug.ae_mgr.preview.update 1

pause
adb shell input keyevent 27

::6400
echo ISO 6400
adb shell setprop vendor.debug.ae_mgr.shutter 469
adb shell setprop vendor.debug.ae_mgr.sensorgain 15872
adb shell setprop vendor.debug.ae_mgr.ispgain 8456

adb shell setprop vendor.debug.ae_mgr.preview.update 1

pause
adb shell input keyevent 27

::9600
echo ISO 9600
adb shell setprop vendor.debug.ae_mgr.shutter 312
adb shell setprop vendor.debug.ae_mgr.sensorgain 15872
adb shell setprop vendor.debug.ae_mgr.ispgain 12684

adb shell setprop vendor.debug.ae_mgr.preview.update 1

pause
adb shell input keyevent 27

::12800
echo ISO 12800
adb shell setprop vendor.debug.ae_mgr.shutter 235
adb shell setprop vendor.debug.ae_mgr.sensorgain 15872
adb shell setprop vendor.debug.ae_mgr.ispgain 16912

adb shell setprop vendor.debug.ae_mgr.preview.update 1

pause
adb shell input keyevent 27
