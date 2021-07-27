adb root
adb remount
adb shell setenforce 0
adb shell rm /data/vendor/camera_dump -rf
adb shell mkdir /data/vendor/camera_dump -p
adb shell setprop vendor.debug.p2s.dsdn.mode 0
adb shell mrdump_tool output-set none


adb shell setprop vendor.debug.camera.p2.dump.filter 3


echo #########################
echo #########################
adb shell setprop vendor.mfll.log_level 3

adb shell "kill $(pidof cameraserver)"
adb shell "kill $(pidof camerahalserver)"

adb shell setprop vendor.mfll.dump.sim 1

echo #Tuning
adb shell setprop vendor.mfll.dump.mixer.out 1


adb shell setprop vendor.debug.camera.p2.dump 1
echo #########################
echo from capture DCE
echo #########################
adb shell setprop vendor.debug.camera.dump.JpegNode 1
adb shell setprop persist.vendor.dp.dumpreg.check 1
adb shell setprop vendor.debug.camera.SttBufQ.enable 60
adb shell setprop vendor.debug.camera.ufo_off 1
adb shell setprop vendor.debug.camera.p2.dump 1
adb shell setprop vendor.debug.camera.dump.campipe 1
adb shell setprop vendor.debug.camera.dump.p2 1
adb shell setprop vendor.debug.camera.dump.p2.ext.img3o 1
adb shell setprop vendor.debug.camera.img3o.dump 0
adb shell setprop vendor.debug.camera.copy.p1.lsc 1
adb shell setprop vendor.debug.hal3av3.forcedump 1
adb shell setprop vendor.debug.aaa.perframe_prop.enable 1

adb shell setprop mtk.camera.app.keycode.enable 1
adb shell setprop vendor.dp.dumpreg.enable 2
adb shell setprop vendor.debug.camera.dump.p2.debuginfo 1
adb shell setprop vendor.debug.camera.dump.lsc2 1
adb shell setprop vendor.debug.camera.AAO.dump 1

adb shell setprop vendor.flash_bin_en 1
adb shell setprop vendor.flash_pf_bmp_en 1
adb shell setprop vendor.flash_dump.enable 1
adb shell setprop vendor.isp.ynr_tbl.dump 1

adb shell setprop vendor.isp.ltm_tbl.dump 1
adb shell setprop vendor.isp.dce_tbl.dump 1
adb shell setprop vendor.isp.faceinfo.dump 1
adb shell setprop vendor.isp.ynr_map.dump 1
REM MSNR LCSO
adb shell setprop vendor.debug.camera.msnr.ndd.l1 1
adb shell setprop vendor.debug.camera.msnr.ndd.l2 1

echo #dump rrzo/yuvo
adb shell setprop vendor.debug.camera.bss.dump 1

echo #LCE
adb shell setprop vendor.isp.lcesho_tbl.dump 1
echo  #dump CALTM NDD
adb shell setprop vendor.mfll.dump.mixer 1
adb shell setprop vendor.debug.camera.dump.p2 1

echo # dump p1.reg
adb shell setprop vendor.debug.isp.dumpcq.enable 1

echo # dump p2.reg
echo # dump p2.reg 0:disable, 1:streaming, 2:capture
adb shell setprop vendor.imgsys.bittruedump.enable 2

echo echo # dump MFB.reg
adb shell setprop vendor.debug.camera.dump.mfb.tuning 1
adb shell setprop vendor.debug.camera.dump.mss.tuning 1
echo # dump MSS.reg
adb shell setprop vendor.msf.debug.bittruedump.enable 1
adb shell setprop vendor.mss.debug.bittruedump.enable 1

echo #dump msyuv, swmsf, msftbl
adb shell setprop vendor.debug.camera.dump.swmsf 1
adb shell setprop vendor.debug.camera.dump.msyuv 1
adb shell setprop vendor.debug.camera.dump.mfbtbl 1

echo # ISP iso interpolation
rem adb shell setprop vendor.isp.interp.ctrl 0

echo # Enable debug info in EXIF
adb shell setprop vendor.debug.camera.dbginfo 1
adb shell setprop vendor.debug.camera.exif.update 1

echo # dump haljpg
adb shell setprop vendor.debug.camera.haljpeg 1
rem #for AWB
adb shell setprop vendor.debug.awb.multiFrmUpdate.disable 1

rem #YUVO file name
adb shell setprop vendor.debug.camera.IspTuningDataPackUtil.DownSample 0
adb shell setprop vendor.debug.camera.dump.JpegNode 1
rem adb shell setprop debug.camera.imgBuf.enFC 1
rem AINR dump
adb shell setprop vendor.debug.camera.ainr.dump 1
adb shell setprop vendor.dump.ainr.buffer 1
adb shell setprop vendor.bittrue.ainr.en 1
adb shell setprop vendor.nrcore.dump.enable 1

echo ###### temp solution: kill camera service for dump reg
adb shell "kill $(pidof camerahalserver)"
adb shell "kill $(pidof cameraserver)"
adb shell pkill camera*

echo #######################
echo # start camera app
echo #######################
rem adb shell am start com.mediatek.camera

pause
