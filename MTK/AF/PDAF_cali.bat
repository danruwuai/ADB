@echo off

echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0
echo 输入AF_log开关
CALL "%~dp0\af_log.bat"
timeout 4
echo +++++++++++++++++++++++++++++++++++++
echo 开启相机
adb shell am start -a android.media.action.STILL_IMAGE_CAMERA --ei android.intent.extras.Camera_FACING 0

echo +++++++++++++++++++++++++++++++++++++
echo 打开MTK logger，记录log信息
echo 开启后请输入按键继续
pause
echo +++++++++++++++++++++++++++++++++++++
echo 输入Inf端OTP数据
set /p Inf_OTP=
echo +++++++++++++++++++++++++++++++++++++
echo 输入Mac端OTP数据
set /p Mac_OTP=
set /a Mac=%Mac_OTP%+8
echo +++++++++++++++++++++++++++++++++++++
echo 开始手动设置Position
adb shell setprop vendor.debug.af_motor.disable 1
rem Postion对应位置
set /a step=(Mac-Inf_OTP)/9
for /l %%p in (%Inf_OTP%,%step%,%Mac%) do (
::set a=%%p 
echo +++++++++++++++++++++++++++++++++++++
echo 移动Postion到%%p
adb shell setprop vendor.debug.af_motor.position %%p
timeout 3 >nul 
)
adb shell setprop vendor.debug.af_motor.disable 0
timeout 4 >nul