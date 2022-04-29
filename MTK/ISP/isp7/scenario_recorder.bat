adb wait-for-device root
adb remount
adb shell setenforce 0
adb shell setprop vendor.debug.mapping_mgr.enable 1
adb shell setprop vendor.debug.idxcache.log 1
adb shell setprop vendor.debug.aaa.perframe_prop.enable 1
adb shell setprop vendor.dataprovider.debug.enable 1
adb shell setprop vendor.debug.camera.scenarioRecorder.enable 1
adb shell pkill camera*
