rem : push config file
adb push ndd_autogen_cfg.cfg data/vendor/camera_dump
adb shell chmod 777 data/vendor/camera_dump/ndd_autogen_cfg.cfg
adb shell setprop vendor.debug.ndd.cfgpath "/data/vendor/camera_dump/ndd_autogen_cfg.cfg"
adb shell setprop vendor.debug.ndd.prv_ready 0