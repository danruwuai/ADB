:How To Use ~~~ 

:If the filename format is like that *_4656x3496_5824_0.raw
:You can use format : UniUnpackRaw.exe [filename]
:
:Else
: use format : UniUnpackRaw.exe [filename] [width] [Height] [Stride] [Bayer order]


::UniUnpackRaw.exe 140935214-0183-0183-BSS-BSS_IN-yplane-PW1472-PH1080-BW1840__1444x1080_10_s0.packed_word 1444 1080 10 0
::UniUnpackRaw.exe CaptureISO400_1920x1080_3840_1.raw
::UniUnpackRaw.exe CaptureISO1600_1920x1080_3840_1.raw
::UniUnpackRaw.exe CaptureISO3200_1920x1080_3840_1.raw
::UniUnpackRaw.exe CaptureISO4800_1920x1080_3840_1.raw
::UniUnpackRaw.exe CaptureISO6400_1920x1080_3840_1.raw
::UniUnpackRaw.exe CaptureISO9600_1920x1080_3840_1.raw
::UniUnpackRaw.exe CaptureISO10000_1920x1080_3840_1.raw

rem unpack6589raw.exe PASS1_FULLRAW_0_4176x3088_5220_3.raw 4176 3088 10 2
echo off
setlocal enabledelayedexpansion

set "raw_dir=.\"
set packed_word=.packed_word
set raw=.raw
set bmp=.bmp

echo off
del %raw_dir%*%raw%
del %raw_dir%*%bmp%
del %cd%\RAW_RESULT\
:for
::for循环判断是否是raw
::for /R %raw_dir% %%i in (*__*.packed_word) do (
for %%i in (%raw_dir%*__*) do (
::读取raw的文件名
	set "str=%%~ni"
	echo 简略文件名：!str!
	set count=-1
	set count1=0
	set count2=1
	set count3=2
	set count4=3
	set count5=4
::pause

:loop_Bayer_order
::获取Pixel_id
if "!str:~%count%,1!"=="" (
	goto end
) else if "!str:~%count%,1!"=="_" (
	if "!str:~%count1%,1!"=="s" (
		goto break
	) else (
		set Pixel_id=!str:~%count1%,1!
	)
	set /a count-=1
	set /a count1-=1
	set /a count2-=1
	set /a count3-=1
	set /a count4-=1
	set /a count5-=1
	echo Pixel_id is !Pixel_id!
	goto loop_Stride
) else (
	set /a count-=1
	set /a count1-=1
	set /a count2-=1
	set /a count3-=1
	set /a count4-=1
	set /a count5-=1
	goto loop_Bayer_order
)

::获取bitdepth
:loop_Stride
if "!str:~%count%,1!"=="" (
	goto end
) else if "!str:~%count%,1!"=="_" (
	if "!str:~%count3%,1!"=="_" (
		set bitdepth=!str:~%count1%,2!
		set /a count-=1
		set /a count1-=1
		set /a count2-=1
		set /a count3-=1
		set /a count4-=1
		set /a count5-=1
		echo bitdepth is !bitdepth!
		goto loop_height
	) else if "!str:~%count2%,1!"=="_" (
		set bitdepth=!str:~%count1%,1!
		set /a count-=1
		set /a count1-=1
		set /a count2-=1
		set /a count3-=1
		set /a count4-=1
		set /a count5-=1
		echo bitdepth is !bitdepth!
		goto loop_height
	)else (
		set /a count-=1
		set /a count1-=1
		set /a count2-=1
		set /a count3-=1
		set /a count4-=1
		set /a count5-=1
		goto loop_Stride
	)
)else (
	set /a count-=1
	set /a count1-=1
	set /a count2-=1
	set /a count3-=1
	set /a count4-=1
	set /a count5-=1
	goto loop_Stride
)

::获取height
:loop_height
if "!str:~%count%,1!"=="" (
	goto end
) else if "!str:~%count%,1!"=="x" (
	if "!str:~%count5%,1!"=="_" (
		set height=!str:~%count1%,4!
		set /a count-=1
		set /a count1-=1
		set /a count2-=1
		set /a count3-=1
		set /a count4-=1
		set /a count5-=1
		echo !height!
		goto loop_width
	) else if "!str:~%count4%,1!"=="_" (
		set height=!str:~%count1%,3!
		set /a count-=1
		set /a count1-=1
		set /a count2-=1
		set /a count3-=1
		set /a count4-=1
		set /a count5-=1
		echo !height!
		goto :loop_width
	)else (
		set /a count-=1
		set /a count1-=1
		set /a count2-=1
		set /a count3-=1
		set /a count4-=1
		set /a count5-=1
		goto loop_height
	)
) else (
	set /a count-=1
	set /a count1-=1
	set /a count2-=1
	set /a count3-=1
	set /a count4-=1
	set /a count5-=1
	goto loop_height
)

::获取width
:loop_width
if "!str:~%count%,1!"=="" (
	goto end
) else if "!str:~%count%,1!"=="_" (
	if "!str:~%count5%,1!"=="x" (
		set width=!str:~%count1%,4!
		set /a count-=1
		set /a count1-=1
		set /a count2-=1
		set /a count3-=1
		set /a count4-=1
		set /a count5-=1
		echo !width!
		goto other
	) else if "!str:~%count4%,1!"=="x" (
		set width=!str:~%count1%,3!
		set /a count-=1
		set /a count1-=1
		set /a count2-=1
		set /a count3-=1
		set /a count4-=1
		set /a count5-=1
		echo !width!
		goto other
	)else (
		set /a count-=1
		set /a count1-=1
		set /a count2-=1
		set /a count3-=1
		set /a count4-=1
		set /a count5-=1
		goto loop_width
	)
) else (
	set /a count-=1
	set /a count1-=1
	set /a count2-=1
	set /a count3-=1
	set /a count4-=1
	set /a count5-=1
	goto loop_width
)
:other
echo packed_word is !str!%packed_word%
.\UniUnpackRaw.exe !str!%packed_word% !width! !height! !bitdepth! !Pixel_id!
mkdir %cd%\RAW_RESULT
move %raw_dir%*%raw% %cd%\RAW_RESULT\
move %raw_dir%*%bmp% %cd%\RAW_RESULT\
:break
move !str!*%packed_word% %cd%\RAW_RESULT\
goto for

)

::del /Q %path%*
echo 结束
move %cd%\RAW_RESULT\*%packed_word% %cd%
pause


:end
exit /B 0