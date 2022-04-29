adb root
adb shell setenforce 0
adb shell logcat -G 80M
adb shell "ps -A | grep cameraserver | grep -v grep | sed 's/ [ ]*/:/g' | cut -d: -f2 | xargs kill -9"
adb shell setprop persist.vendor.mtk.camera.log_level 3
adb shell setprop vendor.debug.aaa.pvlog.enable 1
adb shell setprop vendor.debug.paramctrl.enable 1
adb shell setprop vendor.profile.paramctrl.enable 1
adb shell setprop vendor.debug.isp_tuning_mgr.enable 1
adb shell setprop vendor.debug.isp_mgr.enable 1
adb shell setprop vendor.debug.mapping_mgr.enable 3
adb shell setprop persist.vendor.mtk.camera.log_level 3
adb shell setprop persist.vendor.mtk.debug.camera.log 3
adb shell setprop persist.mtk.camera.log_level 3
adb shell setprop persist.vendor.debug.camera.log 3 
adb shell setprop persist.vendor.debug.camera.dbginfo 1
adb shell setprop vendor.debug.camera.dump.JpegNode 1
adb shell setprop vendor.debug.camera.dump.JpegNode 1
adb shell setprop persist.vendor.dp.dumpreg.check 1
adb shell setprop vendor.debug.camera.SttBufQ.enable 60
adb shell setprop vendor.debug.camera.ufo_off 1
adb shell setprop vendor.debug.camera.p2.dump 1
adb shell setprop vendor.debug.camera.dump.campipe 1
adb shell setprop vendor.debug.camera.dump.p2 1
adb shell setprop vendor.debug.camera.dump.p2.ext.img3o 1
adb shell setprop vendor.debug.camera.img3o.dump 1
adb shell setprop vendor.debug.camera.copy.p1.lsc 1
adb shell setprop vendor.debug.hal3av3.forcedump 1
adb shell setprop vendor.debug.aaa.perframe_prop.enable 1

adb shell logcat -G 80M

::adb shell setprop log.tag.PQ VERBOSE
::adb shell setprop vendor.debug.dp.dumpregister.enable 3
::ËÑË÷ "Mod:NBC_ANR"¹Ø¼ü×Ö
adb shell logcat | findstr "Mod:UDM"

pause