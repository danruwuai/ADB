1.拷贝build_so.sh到k16 code根目录
2.打开终端输入
chmod a+x build_so.sh
3.终端输入命令编译so
./build_so.sh
4.对应数字  1 编译evergreen  2 编译evergo 3 编译evergo_cn
5.如果只编译对应sensor，可以在build_so.sh里修改如下：
			mmm vendor/mediatek/proprietary/custom/evergreen/hal/imgsensor/ver1/
增加		mmm vendor/mediatek/proprietary/custom/evergreen/hal/imgsensor/ver1/ov50c40ofilm_mipi_raw