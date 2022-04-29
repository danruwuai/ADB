adb root
adb shell setenforce 0

adb shell setprop vendor.debug.p2g.force.buffer.round 5
adb shell setprop persist.vendor.camera3.pipeline.bufnum.base.rsso 15
adb shell setprop persist.vendor.camera3.pipeline.bufnum.base.rrzo 10

rem : thread number of NDD
adb shell setprop vendor.debug.ndd.thdnum 2

rem : enable sub folder
adb shell setprop vendor.debug.ndd.subdir 1
adb shell setprop vendor.debug.ndd.debuging 1
adb shell setprop vendor.debug.camera.dbginfo 1

rem : enable p1 reg
adb shell setprop vendor.debug.camera.pipemgr.bufdump 1

rem : use FileCache and set size = 4GB
adb shell setprop vendor.debug.camera.imgBuf.enFC 1
adb shell setprop vendor.debug.file_cache.storage_size 5368709120

rem : enable screen recorder
adb shell setprop vendor.debug.camera.scenarioRecorder.enable 1

rem : enable cfg of on-device tuning
adb shell setprop vendor.debug.ndd.gen_cfg 1

rem : dump color param
adb shell setprop vendor.debug.fwcolorparam.dump 1 

rem : ???
adb shell setprop vendor.camera.force_rst_mmap.en 1

rem : re-start camera
adb shell pkill camera
adb shell logcat -c
adb shell logcat -G 30M