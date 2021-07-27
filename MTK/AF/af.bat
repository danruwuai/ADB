@echo off

echo +++++++++++++++++++++++++++++++++++++
echo 打开AF LOG
echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0
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
adb shell setprop debug.cam.draw.ctrl 'DAC,MAGIC,ISO'
adb shell setprop vendor.debug.drv.ccu_drv 4
adb shell setprop vendor.debug.ccuif.ccu_drv 4
adb shell setprop vendor.drv.ccu.log.ccuflowaf 1
adb shell setprop vendor.drv.ccu.log.ccuafalgo 1
::adb shell setprop vendor.debug.ae.log.enable 1
adb shell setprop vendor.debug.hal3a.taskmgr 1
adb shell setprop vendor.debug.hal3a.task 1
adb shell setprop vendor.debug.hal3av3.log 3
adb shell setprop vendor.debug.thread_raw.log 1
adb shell svc power stayon true
adb shell settings put system screen_off_timeout 1800000
adb shell sync
adb shell sync
adb shell sync
adb shell pkill camera*
adb logcat -G 16m
adb logcat -c
echo +++++++++++++++++++++++++++++++++++++
echo 输出 log结束
echo +++++++++++++++++++++++++++++++++++++
timeout 2 > nul