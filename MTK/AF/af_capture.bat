echo 输入测试远焦数据
set /p Inf_OTP=
echo +++++++++++++++++++++++++++++++++++++
echo 输入测试近焦数据
set /p Mac_OTP=
set /a Mac=%Mac_OTP%
echo +++++++++++++++++++++++++++++++++++++
echo 开始手动设置Position
adb shell setprop vendor.pd.offset.comp 0
adb shell setprop vendor.debug.af.localfullscan 0
adb shell setprop vendor.debug.af.localfullscan.auto 0
adb shell setprop vendor.debug.ae_mgr.enable 1
adb shell setprop vendor.debug.af_motor.disable 1
adb shell setprop vendor.debug.af_motor.position 5
adb shell sleep 0.5
adb shell setprop vendor.debug.ae_mgr.lock 1
rem Postion对应位置
echo 输入测试Step数据
set /p step=
for /l %%p in (%Inf_OTP%,%step%,%Mac%) do (
::set a=%%p 
echo +++++++++++++++++++++++++++++++++++++
echo 移动Postion到%%p
adb shell setprop vendor.debug.af_motor.position %%p
timeout 2
adb shell input keyevent 27
)
adb shell input keyevent KEYCODE_BACK
adb shell setprop vendor.debug.af_motor.disable 0
adb shell setprop vendor.debug.ae_mgr.lock 0
adb shell setprop vendor.debug.ae_mgr.enable 0
ping 127.0.0.1 -n 3