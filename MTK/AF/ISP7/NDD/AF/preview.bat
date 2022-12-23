adb shell rm -rf data/vendor/camera_dump/default_dump/*
adb shell setprop vendor.debug.ndd.prv_ready 1
timeout 1
adb shell setprop vendor.debug.debug.ndd.prv_ready 1
adb pull data/vendor/camera_dump/default_dump/ %cd%