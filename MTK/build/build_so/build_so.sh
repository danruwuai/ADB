echo "Which code do you want to compile?"
echo "1 ---- evergreen"
echo "2 ---- evergo"
echo "3 ---- evergo_cn"
select name in "1" "2" "3"
do
    case $name in
        "1")
            echo "build evergreen"
			source ./build/envsetup.sh
			lunch evergreen-userdebug
			export WT_COMPILE_FACTORY_VERSION=no
			export MIUI_BUILD_REGION=
			mmm vendor/mediatek/proprietary/custom/evergreen/hal/imgsensor/ver1/
            break
            ;;
        "2")
            echo "build evergo"
			source ./build/envsetup.sh
			lunch evergo-userdebug
			export WT_COMPILE_FACTORY_VERSION=no
			export MIUI_BUILD_REGION=
			mmm vendor/mediatek/proprietary/custom/evergo/hal/imgsensor/ver1/
            break
            ;;
        "3")
            echo "build evergo_cn"
			source ./build/envsetup.sh
			lunch evergo-userdebug
			export WT_COMPILE_FACTORY_VERSION=no
			export MIUI_BUILD_REGION=CN
			mmm vendor/mediatek/proprietary/custom/evergo/hal/imgsensor/ver1_cn/
            break
            ;;
        *)
            echo "Wrong input, please try again"
    esac
done
