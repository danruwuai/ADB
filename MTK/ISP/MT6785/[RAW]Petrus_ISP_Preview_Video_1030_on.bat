adb root
adb wait-for-device
adb shell am force-stop com.mediatek.camera
adb shell rm /data/vendor/camera_dump/*.* -rf
adb shell mkdir /data/vendor/camera_dump -p
adb shell setenforce 0

@REM Initial
adb shell setprop vendor.debug.camera.SttBufQ.enable 60
adb shell setprop vendor.debug.camera.copy.p1.lsc 1
adb shell setprop vendor.debug.p2f.dump.enable 1
adb shell setprop vendor.debug.p2f.dump.mode 1
adb shell setprop vendor.debug.camera.dump.en 1
adb shell setprop vendor.debug.camera.ufo_off 1
adb shell setprop persist.vendor.dp.dumpreg.check 1
adb shell setprop vendor.debug.hal3av3.forcedump 1
adb shell setprop vendor.debug.isp.dumpcq.framecnt 1200
adb shell setprop vendor.debug.isp.dumpcq.enable 1
adb shell setprop vendor.debug.fpipe.force.img3o 1
adb shell setprop vendor.debug.fpipe.frame.setting 1
adb shell setprop vendor.debug.aaa.perframe_prop.enable 1

adb shell setprop vendor.debug.camera.dump.campipe 1 

adb shell setprop vendor.debug.camera.imgBuf.enFC 1
adb shell setprop vendor.debug.dp.dumpregister.enable 1
adb shell setprop vendor.debug.shot.forcenr 0

adb shell setprop vendor.debug.mask.fpipe.mss 0
adb shell setprop vendor.debug.mask.fpipe.msf 0
adb shell setprop vendor.debug.camera.dump.msyuv 1

@REM adb
adb shell setprop vendor.dp.dumpreg.enable 1
adb shell setprop vendor.debug.camera.dump.p1.imgo 1
adb shell setprop vendor.debug.camera.dump.isp.preview 1
adb shell setprop vendor.isp.bpc_tbl.dump 1
adb shell setprop vendor.isp.bpc_tbl.dump.preview 1
adb shell setprop vendor.debug.camera.dump.p1.tsfo 1
adb shell setprop vendor.debug.camera.dump.p1.lsc 1
adb shell setprop vendor.debug.camera.dump.lsc2 1
adb shell setprop vendor.debug.camera.AAO.dump.preview 1
adb shell setprop vendor.debug.camera.preview.dump 1

adb shell setprop vendor.debug.camera.dump.p2.debuginfo 1

@REM LCE DCE LTM
adb shell setprop vendor.isp.dce_tbl.dump.preview 1
adb shell setprop vendor.isp.ltm_tbl.dump.preview 1
adb shell setprop vendor.isp.lcesho_tbl.dump.preview 1

@REM YNR tbl
adb shell setprop vendor.isp.ynr_map.dump.preview 1
adb shell setprop vendor.isp.ynr_tbl.dump.preview 1
adb shell setprop vendor.isp.faceinfo.dump.preview 1

adb shell setprop vendor.imgsys.bittruedump.enable 1

rem # force ISO setting
rem adb shell setprop vendor.debug.camera.cammode 1
rem adb shell setprop vendor.debug.hal3av3.manual 1
rem adb shell setprop vendor.debug.camera.isospeed 200

adb shell pkill camerahalserver
::rem  driver need set 1 for preview dump, also need kill server

pause