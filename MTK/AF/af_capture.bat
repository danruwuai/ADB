echo �������Զ������
set /p Inf_OTP=
echo +++++++++++++++++++++++++++++++++++++
echo ������Խ�������
set /p Mac_OTP=
set /a Mac=%Mac_OTP%
echo +++++++++++++++++++++++++++++++++++++
echo ��ʼ�ֶ�����Position
adb shell setprop vendor.pd.offset.comp 0
adb shell setprop vendor.debug.af.localfullscan 0
adb shell setprop vendor.debug.af.localfullscan.auto 0
adb shell setprop vendor.debug.ae_mgr.enable 1
adb shell setprop vendor.debug.af_motor.disable 1
adb shell setprop vendor.debug.af_motor.position 5
adb shell sleep 0.5
adb shell setprop vendor.debug.ae_mgr.lock 1
rem Postion��Ӧλ��
echo �������Step����
set /p step=
for /l %%p in (%Inf_OTP%,%step%,%Mac%) do (
::set a=%%p 
echo +++++++++++++++++++++++++++++++++++++
echo �ƶ�Postion��%%p
adb shell setprop vendor.debug.af_motor.position %%p
timeout 2
adb shell input keyevent 27
)
adb shell input keyevent KEYCODE_BACK
adb shell setprop vendor.debug.af_motor.disable 0
adb shell setprop vendor.debug.ae_mgr.lock 0
adb shell setprop vendor.debug.ae_mgr.enable 0
ping 127.0.0.1 -n 3