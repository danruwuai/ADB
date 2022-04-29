adb root
adb shell setenforce 0

rem : thread number of NDD
adb shell setprop vendor.debug.ndd.thdnum 2

rem : enable sub folder
adb shell setprop vendor.debug.ndd.subdir 0
adb shell setprop vendor.debug.ndd.debuging 1

rem : use FileCache and set size = 4GB
adb shell setprop vendor.debug.camera.imgBuf.enFC 1
adb shell setprop vendor.debug.file_cache.storage_size 5368709120

rem : re-start camera
adb shell pkill camera
adb shell logcat -c
adb shell logcat -G 30M