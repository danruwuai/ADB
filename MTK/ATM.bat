@echo off
:android
::cls
adb wait-for-device
set/p x=choose：1--Android Old 2--Android NEW 3--查询的模块： e--Exit：
if "%x%"=="1" goto beginOld
if "%x%"=="2" goto beginNew
if "%x%"=="3" goto log
if "%x%"=="e" goto Exit
goto android

:beginOld
adb wait-for-device root
adb remount
adb shell setenforce 0
adb shell setprop persist.mtk.camera.log_level 3
adb shell setprop debug.mapping_mgr.enable 3
adb shell setprop debug.paramctrl.enable 1
adb shell setprop debug.camera.dbginfo 1
adb shell setprop debug.camera.dump.p2.debuginfo 1
adb shell pkill camera*
adb logcat -G 16m
adb logcat -c
timeout 5 >nul
adb shell am start -a android.media.action.STILL_IMAGE_CAMERA --ei android.intent.extras.Camera_FACING 0
goto begin

:beginNew
adb wait-for-device root
adb remount
adb shell setenforce 0
adb shell setprop persist.vendor.mtk.camera.log_level 3
adb shell setprop vendor.debug.mapping_mgr.enable 3
adb shell setprop vendor.debug.paramctrl.enable 1
adb shell setprop vendor.debug.camera.dbginfo 1
adb shell setprop vendor.debug.camera.dump.p2.debuginfo 1
adb shell setprop vendor.debug.aaa.perframe_prop.enable 1
adb shell setprop vendor.profile.paramctrl.enable 1
adb shell pkill camera*
adb logcat -G 16m
adb logcat -c
timeout 5 >nul
adb shell am start -a android.media.action.STILL_IMAGE_CAMERA --ei android.intent.extras.Camera_FACING 0
goto begin

:Exit
echo 你选择的是Exit
exit

:log 
adb logcat -G 16m
adb logcat -c
goto begin

@echo off
:begin
::cls
set/p c=查询的模块：

if "%c%"=="DBS" goto DBS
if "%c%"=="dbs" goto DBS

if "%c%"=="ADBS" goto ADBS
if "%c%"=="adbs" goto ADBS

if "%c%"=="OBC" goto OBC
if "%c%"=="obc" goto OBC

if "%c%"=="BPC" goto BPC
if "%c%"=="bpc" goto BPC

if "%c%"=="NR1" goto NR1
if "%c%"=="nr1" goto NR1

if "%c%"=="PDC" goto PDC
if "%c%"=="pdc" goto PDC

if "%c%"=="RMM" goto RMM
if "%c%"=="rmm" goto RMM

if "%c%"=="RNR" goto RNR
if "%c%"=="rnr" goto RNR

if "%c%"=="SL2F" goto SL2F
if "%c%"=="sl2f" goto SL2F

if "%c%"=="SL2" goto SL2
if "%c%"=="sl2" goto SL2

if "%c%"=="UDM" goto UDM
if "%c%"=="udm" goto UDM

if "%c%"=="ANR" goto ANR
if "%c%"=="anr" goto ANR

if "%c%"=="ANR2" goto ANR2
if "%c%"=="anr2" goto ANR2

if "%c%"=="CCR" goto CCR
if "%c%"=="ccr" goto CCR

if "%c%"=="ABF" goto ABF
if "%c%"=="abf" goto ABF

if "%c%"=="HFG" goto HFG
if "%c%"=="hfg" goto HFG

if "%c%"=="EE" goto EE
if "%c%"=="ee" goto EE

if "%c%"=="NR3D" goto NR3D
if "%c%"=="nr3d" goto NR3D

if "%c%"=="MFB" goto MFB
if "%c%"=="mfb" goto MFB

if "%c%"=="MIXER3" goto MIXER3
if "%c%"=="mixer3" goto MIXER3

if "%c%"=="CCM" goto CCM
if "%c%"=="ccm" goto CCM

if "%c%"=="COLOR" goto COLOR
if "%c%"=="color" goto COLOR

if "%c%"=="PCA" goto PCA
if "%c%"=="pca" goto PCA

if "%c%"=="AE" goto AE
if "%c%"=="ae" goto AE

if "%c%"=="AF" goto AF
if "%c%"=="af" goto AF

if "%c%"=="AWB" goto AWB
if "%c%"=="awb" goto AWB

if "%c%"=="FLASH_AE" goto Flash_AE
if "%c%"=="flash_ae" goto Flash_AE

if "%c%"=="FLASH_AWB" goto Flash_AWB
if "%c%"=="flash_awb" goto Flash_AWB

if "%c%"=="FLASH_CALIBRATION" goto Flash_Calibration
if "%c%"=="flash_calibration" goto Flash_Calibration

if "%c%"=="LCE" goto LCE
if "%c%"=="lce" goto LCE

if "%c%"=="MFNR" goto MFNR
if "%c%"=="mfnr" goto MFNR

if "%c%"=="SWNR" goto SWNR
if "%c%"=="swnr" goto SWNR

if "%c%"=="CA_LTM" goto CA_LTM
if "%c%"=="ca_ltm" goto CA_LTM

if "%c%"=="CLEARZOOM" goto ClearZoom
if "%c%"=="clearzoom" goto ClearZoom

if "%c%"=="GMA" goto GMA
if "%c%"=="gma" goto GMA

if "%c%"=="LCE_LINK" goto LCE_LINK
if "%c%"=="lce_link" goto LCE_LINK

if "%c%"=="TBL" goto TBL
if "%c%"=="tbl" goto TBL

if "%c%"=="COLOR_PARAM" goto COLOR_PARAM
if "%c%"=="COLOR_PARAM" goto COLOR_PARAM

if "%c%"=="SWNR_THRES" goto SWNR_THRES
if "%c%"=="swnr_thres" goto SWNR_THRES

if "%c%"=="FD_ANR" goto FD_ANR
if "%c%"=="fd_anr" goto FD_ANR

if "%c%"=="DCE" goto DCE
if "%c%"=="dce" goto DCE
goto begin

:DBS
adb logcat | findstr -i "Mod:DBS"
goto begin

:ADBS
adb logcat | findstr -i "Mod:ADBS"
goto begin

:OBC
adb logcat | findstr -i "Mod:OBC"
goto begin

:BPC
adb logcat | findstr -i "Mod:BNR_BPC"
goto begin

:NR1
adb logcat | findstr -i "Mod:BNR_NR1"
goto begin

:PDC
adb logcat | findstr -i "Mod:BNR_PDC"
goto begin

:RMM
adb logcat | findstr -i "Mod:RMM"
goto begin

:RNR
adb logcat | findstr -i "Mod:RNR"
goto begin

:SL2F
adb logcat | findstr -i "Mod:SL2F"
goto begin

:SL2
adb logcat | findstr -i "Mod:SL2"
goto begin

:UDM
adb logcat | findstr -i "Mod:UDM"
goto begin

:ANR
adb logcat | findstr -i "Mod:NBC_ANR"
goto begin

:ANR2
adb logcat | findstr -i "Mod:NBC2_ANR2"
goto begin

:CCR
adb logcat | findstr -i "Mod:NBC2_CCR"
goto begin

:ABF
adb logcat | findstr -i "Mod:NBC2_ABF"
goto begin

:HFG
adb logcat | findstr -i "Mod:HFG"
goto begin

:EE
adb logcat | findstr -i "Mod:EE"
goto begin

:NR3D
adb logcat | findstr -i "Mod:NR3D"
goto begin

:MFB
adb logcat | findstr -i "Mod:MFB"
goto begin

:MIXER3
adb logcat | findstr -i "Mod:MIXER3"
goto begin

:CCM
adb logcat | findstr -i "Mod:CCM"
goto begin

:COLOR
adb logcat | findstr -i "Mod:COLOR("
goto begin

:PCA
adb logcat | findstr -i "Mod:PCA"
goto begin

:AE
adb logcat | findstr -i "Mod:AE"
goto begin

:AF
adb logcat | findstr -i "Mod:AF"
goto begin

:AWB
adb logcat | findstr -i "Mod:AWB"
goto begin

:Flash_AE
adb logcat | findstr -i "Mod:Flash_AE"
goto begin

:Flash_AWB
adb logcat | findstr -i "Mod:Flash_AWB"
goto begin

:Flash_Calibration
adb logcat | findstr -i "Mod:Flash_Calibration"
goto begin

:LCE
adb logcat | findstr -i "Mod:LCE"
goto begin

:MFNR
adb logcat | findstr -i "Mod:MFNR"
goto begin

:SWNR
adb logcat | findstr -i "Mod:SWNR"
goto begin

:CA_LTM
adb logcat | findstr -i "Mod:CA_LTM"
goto begin

:ClearZoom
adb logcat | findstr -i "Mod:ClearZoom"
goto begin

:GMA
adb logcat | findstr -i "Mod:GMA"
goto begin

:LCE_LINK
adb logcat | findstr -i "Mod:NBC_LCE_LINK"
goto begin

:TBL
adb logcat | findstr -i "Mod:NBC_TBL"
goto begin

:COLOR_PARAM
adb logcat | findstr -i "Mod:COLOR_PARAM"
goto begin

:SWNR_THRES
adb logcat | findstr -i "Mod:SWNR_THRES"
goto begin

:FD_ANR
adb logcat | findstr -i "Mod:FD_ANR"
goto begin

:DCE
adb logcat | findstr -i "Mod:DCE"
goto begin
