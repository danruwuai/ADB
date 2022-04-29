rem AE Debug：
adb root
adb shell setenforce 0
adb shell setprop persist.vendor.mtk.camera.log_level 3
adb shell setprop vendor.debug.camera.dbginfo 1
adb shell setprop debug.cam.drawid 1
adb shell setprop vendor.debug.camera.log 1
adb shell setprop vendor.debug.ae.enable 9
adb shell setprop vendor.debug.ae.log.enable 1
adb shell setprop vendor.debug.aaa.pvlog.enable 1
adb shell setprop vendor.debug.isp_mgr.enable 1
adb shell setprop vendor.debug.aaa_sensor_mgr.enable 1
adb shell setprop vendor.debug.ae_mgr.enable 1
adb shell setprop vendor.debug.aecalc.enable 9 

rem LCE DCE Debug：
adb shell setprop vendor.debug.lce.core.enable 1
adb shell setprop vendor.debug.dce.core.enable 1
adb shell setprop vendor.debug.dynamic_lce.log 1
adb shell setprop vendor.debug.paramctrl.enable 1
adb shell setprop vendor.debug.lce.face.log.enable 1

rem MappingMgr Debug：
rem adb shell setprop vendor.debug.mapping_mgr.enable 3 

adb shell pkill camera*

pause


adb shell setenforce 0
adb shell setprop persist.vendor.mtk.camera.log_level 3
adb shell setprop vendor.persist.logmuch.detect false
adb shell setprop vendor.debug.drv.cuu_drv 5
adb shell setprop vendor.debug.ccuif.ccu_drv 5
adb shell setprop vendor.debug.ae.enable 9
adb shell setprop vendor.debug.camera.log 1
adb shell setprop vendor.debug.aaa.pvlog.enable 1
adb shell setprop vendor.debug.mapping_mgr.enable 3
adb shell setprop vendor.debug.camera.dbginfo 1
adb shell setprop debug.cam.drawid 1
adb shell setprop vendor.debug.ae.plineinfo 1
adb shell pkill camera*