rem : push config file
adb push preview_ndd.cfg data/vendor/camera_dump
adb shell chmod 777 data/vendor/camera_dump/preview_ndd.cfg
adb shell setprop vendor.debug.ndd.cfgpath "/data/vendor/camera_dump/preview_ndd.cfg"

rem : initial value for user to check dump data is finished or not
adb shell setprop vendor.debug.ndd.dump_size 1
adb shell setprop vendor.debug.file_cache.dump_size 1

rem : for control dump preview/video
rem : -1 : all dump, 0 : only dump preview, 1 : only dump video
adb shell setprop vendor.debug.ndd.action_enable -1