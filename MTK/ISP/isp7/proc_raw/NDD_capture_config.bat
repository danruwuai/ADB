rem : push config file
adb push capture_ndd.cfg data/vendor/camera_dump
adb shell chmod 777 data/vendor/camera_dump/capture_ndd.cfg
adb shell setprop vendor.debug.ndd.cfgpath "/data/vendor/camera_dump/capture_ndd.cfg"

rem : frame count for preview dump
adb shell setprop vendor.debug.ndd.prv_ready 0

rem : initial value for user to check dump data is finished or not
adb shell setprop vendor.debug.ndd.dump_size 1
adb shell setprop vendor.debug.file_cache.dump_size 1