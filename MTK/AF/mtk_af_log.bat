adb root

adb remount

adb shell setenforce 0

adb shell setprop vendor.debug.hal3av3.log 3

adb shell setprop persist.vendor.mtk.camera.log_level 3

adb shell setprop vendor.debug.af_mgr.enable 1

adb shell setprop vendor.debug.hal3a.taskmgr 1

adb shell setprop vendor.debug.hal3a.task 1

adb shell setprop vendor.debug.thread_raw.log 1

adb shell setprop vendor.debug.resultpool.log 63

adb shell setprop vendor.debug.pdo_mgr.enable 1

adb shell setprop vendor.debug.afo_mgr.enable 1

adb shell setprop vendor.debug.camera.dbginfo 1

adb shell setprop vendor.debug.pd.vpu.enable 1

adb shell setprop debug.af.log.enable 1

adb shell setprop debug.af.enable 1

adb shell setprop vendor.debug.af.log.enable 1

adb shell setprop vendor.debug.af.enable 1

adb shell setprop vendor.debug.af_mgr.enable 1

adb shell setprop vendor.debug.pd.enable 1

adb shell setprop persist.vendor.mtk.camera.log_level 3

adb shell setprop vendor.debug.pd.vpu.log.enable 1

adb shell setprop vendor.debug.camera.af.draw.lens 1

adb shell setprop debug.cam.drawid 1

adb shell setprop vendor.debug.hal3a.taskmgr 1

adb shell setprop vendor.debug.hal3a.task 1

adb shell setprop vendor.debug.hal3av3.log 3

adb shell setprop vendor.debug.thread_raw.log 1

adb shell setprop debug.cam.drawid 1

adb shell sync

adb shell sync

adb shell sync

adb shell pkill camera*

pause