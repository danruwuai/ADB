@echo off

echo +++++++++++++++++++++++++++++++++++++
adb wait-for-device root
adb wait-for-device remount
adb shell setenforce 0
echo ����AF_log����
CALL "%~dp0\af.bat"
timeout 4
echo +++++++++++++++++++++++++++++++++++++
echo �������
cd %~dp0
cd ../..
START CMD /K CALL "%~dp0/Read_otp.bat"
CALL "�������.bat"
echo +++++++++++++++++++++++++++++++++++++
echo �ȴ�5����
timeout /T 300
echo +++++++++++++++++++++++++++++++++++++
rem cls
rem color 0c
rem echo ��MTK logger����¼log��Ϣ
rem echo �����������밴������
rem pause
echo ץȡlog
START CMD /K CALL "%~dp0/adb_logcat.bat"
echo +++++++++++++++++++++++++++++++++++++
rem cls
rem color 07
echo ����Inf��OTP����
set /p Inf_OTP=
echo +++++++++++++++++++++++++++++++++++++
echo ����Mac��OTP����
set /p Mac_OTP=
set /a Mac=%Mac_OTP%+8
echo +++++++++++++++++++++++++++++++++++++
echo ��ʼ�ֶ�����Position
adb shell setprop vendor.debug.ae_mgr.enable 1
adb shell setprop vendor.debug.af_motor.disable 1
adb shell setprop vendor.debug.af_motor.position 5
adb shell sleep 0.5
adb shell setprop vendor.debug.ae_mgr.lock 1
rem Postion��Ӧλ��
set /a step=(Mac-Inf_OTP)/9
for /l %%p in (%Inf_OTP%,%step%,%Mac%) do (
::set a=%%p 
echo +++++++++++++++++++++++++++++++++++++
echo �ƶ�Postion��%%p
adb shell setprop vendor.debug.af_motor.position %%p
timeout 5
)
adb shell input keyevent KEYCODE_BACK
adb shell setprop vendor.pd.dump.enable 0
adb shell setprop vendor.debug.af_motor.disable 0
adb shell setprop vendor.debug.ae_mgr.lock 0
adb shell setprop vendor.debug.ae_mgr.enable 0
ping 127.0.0.1 -n 3
cls
color 0c
echo +++++++++++++++++++++++++++++++++++++
echo ��ر�MTK logger
echo +++++++++++++++++++++++++++++++++++++
timeout 20
taskkill /FI  "WINDOWTITLE eq adb_logcat.bat*" /IM cmd.exe /F
taskkill /FI  "WINDOWTITLE eq Read_otp.bat" /IM cmd.exe /F
timeout 10 >nul