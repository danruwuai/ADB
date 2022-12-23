echo off
setlocal enabledelayedexpansion

set "raw_dir=.\"
set "bmp_dir=.\RAW_RESULT\"
set packed_word=.packed_word
set raw=.raw
set bmp=.bmp

echo off
del %bmp_dir%*%raw%
del %bmp_dir%*%bmp%

::for循环判断是否是raw
:for
for %%i in (%raw_dir%*__*.packed_word) do (
	set "str=%%~ni"
	::读取raw的文件名
	echo 简略文件名：!str!
	
	set count=-1
	set count1=0
	set count2=1
	set count3=2
	set count4=3
	set count5=4
	echo 简略文件名：!str!
:loop_Bayer_order
::获取Pixel_id
if "!str:~%count%,1!"=="" (
	goto end
) else if "!str:~%count%,1!"=="_" (
	if "!str:~%count1%,1!"=="s" (
		set Pixel_id=!str:~%count2%,1!
	) else (
		set Pixel_id0=!str:~%count%,1!
		set Pixel_id=!str:~%count1%,1!
		set Pixel_id2=!str:~%count2%,1!
		set Pixel_id3=!str:~%count3%,1!
		set Pixel_id4=!str:~%count4%,1!
		set Pixel_id5=!str:~%count5%,1!
		echo 111111111111
	)
	set num=!count!
	set /a count-=1
	set /a count1-=1
	set /a count2-=1
	set /a count3-=1
	set /a count4-=1
	set /a count5-=1
	echo Pixel_id0 is ------ !Pixel_id0!
	echo Pixel_id1 is ------ !Pixel_id1!
	echo Pixel_id2 is ------ !Pixel_id2!
	echo Pixel_id3 is ------ !Pixel_id3!
	echo Pixel_id4 is ------ !Pixel_id4!
	echo Pixel_id5 is ------ !Pixel_id5!
	echo num is ------ !num!
	goto loop_Stride
) else (
	set /a count-=1
	set /a count1-=1
	set /a count2-=1
	set /a count3-=1
	set /a count4-=1
	set /a count5-=1
	set Pixel_id0=!str:~%count%,1!
	echo Pixel_id0 is ------ !Pixel_id0!
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
		echo bitdepth is ------ !bitdepth!
		goto loop_height
	) else if "!str:~%count2%,1!"=="_" (
		set bitdepth=!str:~%count1%,1!
		set /a count-=1
		set /a count1-=1
		set /a count2-=1
		set /a count3-=1
		set /a count4-=1
		set /a count5-=1
		echo bitdepth is ------ !bitdepth!
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
		echo height is ------ !height!
		goto loop_width
	) else if "!str:~%count4%,1!"=="_" (
		set height=!str:~%count1%,3!
		set /a count-=1
		set /a count1-=1
		set /a count2-=1
		set /a count3-=1
		set /a count4-=1
		set /a count5-=1
		echo height is ------ !height!
		goto :loop_width
	) else if "!str:~%count3%,1!"=="_" (
		set height=!str:~%count1%,2!
		set /a count-=1
		set /a count1-=1
		set /a count2-=1
		set /a count3-=1
		set /a count4-=1
		set /a count5-=1
		echo height is ------ !height!
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
		echo weidth is ------ !width!
		goto other
	) else if "!str:~%count4%,1!"=="x" (
		set width=!str:~%count1%,3!
		set /a count-=1
		set /a count1-=1
		set /a count2-=1
		set /a count3-=1
		set /a count4-=1
		set /a count5-=1
		echo weidth is ------ !width!
		goto other
	) else if "!str:~%count3%,1!"=="x" (
		set width=!str:~%count1%,2!
		set /a count-=1
		set /a count1-=1
		set /a count2-=1
		set /a count3-=1
		set /a count4-=1
		set /a count5-=1
		echo weidth is ------ !width!
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
echo !str!%packed_word% !width! !height! !bitdepth! !Pixel_id!
.\UniUnpackRaw.exe !str!%packed_word% !width! !height! !bitdepth! !Pixel_id!
mkdir %cd%\RAW_RESULT
move %raw_dir%*%raw% %cd%\RAW_RESULT\
move %raw_dir%*%bmp% %cd%\RAW_RESULT\
::timeout 3
move !str!*%packed_word% %cd%\RAW_RESULT\
	set count=-1
	set count1=0
	set count2=1
	set count3=2
	set count4=3
	set count5=4
goto for

)

::del /Q %path%*
echo 结束
move %cd%\RAW_RESULT\*%packed_word% %cd%
pause


:end
exit /B 0