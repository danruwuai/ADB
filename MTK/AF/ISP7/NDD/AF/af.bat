adb root
adb shell setenforce 0
adb shell setprop vendor.debug.ndd.thdnum 2
adb shell setprop vendor.debug.ndd.subdir 0
adb shell setprop vendor.debug.camera.pipemgr.bufdump 1
adb shell setprop vendor.debug.camera.imgBuf.enFC 1
adb shell setprop vendor.debug.file_cache.storage_size 5368709120
adb shell setprop vendor.debug.camera.scenarioRecorder.enable 1
adb shell setprop vendor.debug.camera.afdump 1
adb shell pkill camera*
adb shell logcat -c
adb shell logcat -G 3OM
adb push preview_ndd.cfg data/vendor/camera_dump
adb shell chmod 777 data/vendor/camera_dump/preview_ndd.cfg
adb shell setprop vendor.debug.ndd.cfgpath "/data/vendor/camera_dump/preview_ndd.cfg"