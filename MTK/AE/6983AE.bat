adb root
adb shell setenforce 0
adb shell setprop persist.vendor.mtk.camera.log_level 3
adb shell setprop vendor.debug.aaa.perframe_prop.enable 1
adb shell setprop vendor.debug.aaa.pvlog.enable 1
adb shell setprop vendor.debug.ae.log.enable 1
adb shell setprop vendor.debug.ae_mgr.log.level 1
adb shell setprop vendor.debug.camera.dbginfo 1
adb shell setprop debug.cam.drawid 1
adb shell setprop debug.cam.draw.ctrl 'DAC,MAGIC,ISO'

::EVC log
adb shell setprop vendor.debug.ae.log.level 65535
adb shell setprop persist.vendor.mtk.camera.log_level 3
::Hal3A
adb shell setprop vendor.debug.hal3actrl.enable 65535

::hal3a frameinfo
::adb shell setprop vendor.hal3a.frameinfo.debug 1
::Hal3A Policy
adb shell setprop vendor.debug.hal3a.policy 1
::task
adb shell setprop vendor.debug.hal3a.task 1

rem::
adb shell setprop vendor.debug.hal3a.taskmgr.enable 1
adb shell setprop vendor.debug.sensor_mgr.log 1
adb shell setprop vendor.debug.sensor_mgr.name 1
adb shell setprop vendor.debug.sensor_mgr.enable 1

rem::LTM
adb shell setprop vendor.debug.ltm.log.enable 1
rem TNC::

adb shell setprop vendor.debug.tnc.log.enable 1
adb shell setprop vendor.debug.tnc.perframe_prop.enable 1
adb shell setprop vendor.debug.tnc.log.level 65535
adb shell setprop vendor.debug.tnc_alg.log.level 1