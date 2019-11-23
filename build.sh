RED='\033[0;31m'
BLUE='\033[0;34m'
GREEN='\033[0;32m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

function _build_core
{
    cd ./src/core
    
    ./build.sh $1 $2

    cd ../../
}

function _build_lib
{
    cd ./src/lib

    ./build.sh $1 $2

    cd ../../
    
}

function _clean
{
    echo ...clean core build folder
    echo ...clean lib build folder
    echo ...clean general build folder
    rm -rf ./src/core/build/
    rm -rf ./src/lib/build/
    rm -rf ./build
}
    
function _build
{
    case $1 in
        all)
            _clean
            ;;
        clean)
            ;;
        *)
            _abnr_usage
            ;;
    esac
}

function _setup_mac_params
{
    # Executables
    EXECUTABLE="./build/nr"

    # Options
    CONFIG_FILE="run.txt"
    RAW_FORMAT="mipi"
    RAW_EXT=".RAWMIPI"
    COLORSEQ="bggr"
    GLOCATION=rfirst

    # Run time selection
    ANCHOR_IDX=0

    # File names
    ORIGIN_NAME="nr_origin"
    DENOISED_NAME="nr_denoised"
    ORIGIN_GINTERP=$ORIGIN_NAME"_g_interp"
    DENOISED_GINTERP=$DENOISED_NAME"_g_interp"

    # Proc Folders
    PROC_SRC_FOLDER="./data/tmp/src/"
    PROC_RESULT_FOLDER="./data/tmp/dst/"

}

# $1: selection
# $2: run name
function _setup_folder
{
    # All are Mipi, BGGR  
    case $1 in
        hand|handheld100lux)
            SRC_FOLDER="./data/source/handheld_100lux/"
            RESULT_FOLDER="./data/result/handheld_100lux/"
            ;;
        dark|hand2|nightshot2)
            SRC_FOLDER="./data/source/handheld2_nightshot/"
            RESULT_FOLDER="./data/result/handheld2_nightshot/"
            ;;
        tripod|tripod100lux)
            # Mipi, BGGR
            SRC_FOLDER="./data/source/tripod_100lux/"
            RESULT_FOLDER="./data/result/tripod_100lux/"
            ;;
        *)
            _abnr_usage
            return
            ;;
    esac

    if [ -z "$2" ]
    then
        RUN_NAME="run"
    else
        RUN_NAME=$2
    fi

    echo "run name: "$RUN_NAME

    mkdir "./data/result/"
    mkdir $RESULT_FOLDER
    mkdir $RESULT_FOLDER$RUN_NAME
}

function _copy_to_proc_folder
{
    rm -rf "./data/tmp"

    mkdir "./data/tmp"
    mkdir $PROC_SRC_FOLDER
    mkdir $PROC_RESULT_FOLDER

    cp $SRC_FOLDER*$RAW_EXT $PROC_SRC_FOLDER
}

# $1: input directory 
# $2: out directory
# $3: config file path
function _generate_config_file
{
    echo "input directory: " $1
    echo "output directory: " $2
    echo "config file: " $3

    echo "InputDirectoryPath" >> $3
    echo "$1" >> $3

    let _count=0
    for f in $1*$RAW_EXT
    do
        echo InputRaw$_count >> $3
        echo $f
        fname=$(basename $f)
        echo =============
        echo $fname >> $3
        let _count=_count+1
    done

    # count is 1 based
    # let _count=_count+1

    echo "InputNumOfRaws" >> $3
    echo $_count >> $3

    echo "InputRawFormat" >> $3
    echo $RAW_FORMAT >> $3

    echo "InputRawWidth" >> $3
    echo "4032" >> $3

    echo "InputRawHeight" >> $3
    echo "3024" >> $3

    echo "InputAnchorIndex" >> $3
    echo "0" >> $3

    echo "InputRawColorSeq" >> $3
    echo $COLORSEQ >> $3

    echo "OutputAnchorImageName" >> $3
    echo "nr_origin" >> $3

    echo "OutputDirectoryPath" >> $3
    echo $2 >> $3

    echo "OutputRawName" >> $3
    echo "nr_denoised" >> $3

    echo "TestProfilingLoopCount" >> $3
    echo "3" >> $3

    echo "AlgoCtrlProcessingResolution" >> $3
    echo "half" >> $3

    unset _count
}

function _run_mac_unit_test
{
    
    echo -e "\n${PURPLE}$EXECUTABLE $PROC_RESULT_FOLDER$CONFIG_FILE${NC}"  

    export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/etc/halide/bin"
    $EXECUTABLE $PROC_RESULT_FOLDER$CONFIG_FILE

    echo -e "\n${PURPLE}...finished${NC}"  
}

function _copy_from_proc_folder
{
    rm -rf $1
    mkdir $1

    cp $PROC_RESULT_FOLDER$DENOISED_NAME".raw" $1
    cp $PROC_RESULT_FOLDER"run.txt" $1
}

function _decode_all_input_raw
{
    let _idx=0;
    for f in $SRC_FOLDER*$RAW_EXT
    do
        UNPACKED_RAW=$1"/origin_"$_idx.raw
        INTERP_RAW=$1"/origin_g_interp_"$_idx.raw

        ./tools/unpack $f 4032 3024 $RAW_FORMAT $UNPACKED_RAW
        ./tools/interp_g $UNPACKED_RAW 4032 3024 $GLOCATION $INTERP_RAW

        if [ $_idx = $ANCHOR_IDX ]; then
            python tools/save_image.py -i $UNPACKED_RAW -w 4032 -h 3024 
            python tools/save_image.py -i $INTERP_RAW -w 4032 -h 3024
        fi

        let _idx=_idx+1
    done

    unset _idx
}

function _decode_denoised_result
{
    python tools/save_image.py -i $1"/"$DENOISED_NAME".raw" -w 4032 -h 3024 

    ./tools/interp_g $1"/"$DENOISED_NAME".raw" 4032 3024 $GLOCATION $1"/"$DENOISED_GINTERP".raw"
    python tools/save_image.py -i $1"/"$DENOISED_GINTERP".raw" -w 4032 -h 3024
}

function _display_mac_result
{
    echo -e "\n${PURPLE}Open file for comparison${NC}" 

    open \
        $1"/"$DENOISED_GINTERP.png \
        $1"/origin_g_interp_"$ANCHOR_IDX.png \
        $1"/"$DENOISED_NAME.png\
        $1"/origin_"$ANCHOR_IDX.png
}

function _run_mac
{
    echo -e "\n${PURPLE}Running Bayer Denoise on linux/mac${NC}"
    
    _setup_mac_params 

    _setup_folder $1 $2

    _copy_to_proc_folder 

    _generate_config_file $PROC_SRC_FOLDER $PROC_RESULT_FOLDER $PROC_RESULT_FOLDER$CONFIG_FILE

    _run_mac_unit_test

    _copy_from_proc_folder $RESULT_FOLDER$RUN_NAME

    _decode_all_input_raw $RESULT_FOLDER$RUN_NAME

    _decode_denoised_result $RESULT_FOLDER$RUN_NAME

    _display_mac_result $RESULT_FOLDER$RUN_NAME

}

function _copy_from_acuity3
{
    echo $1
    echo pull $1 data from Acuity3
    case $1 in
        dark|monkey|monkey10lux)
            echo remove ./data/result/monkey_10lux/
            rm -rf ./data/result/monkey_10lux/
            echo copy from server
            scp -r acuity@172.31.0.3:/home/acuity/workspace/hh/NoiseReduction/Development/data/result/monkey_10lux/ ./data/result/
            ;;
        bright|light|lightbox500lux)
            rm -rf ./data/result/lightbox_gyro_500lux
            scp -r acuity@172.31.0.3:/home/acuity/workspace/hh/NoiseReduction/Development/data/result/lightbox_gyro_500lux/ ./data/result/
            ;;
    esac
}

function _pull
{
    case $1 in
        acuity3)
            _copy_from_acuity3 $2
            ;;
        device|phone|android)
            _pull_from_device $2
            ;;
    esac

}

function _run
{
    case $1 in
        linux|mac)
            _run_mac $2 $3
            ;;
        arm)
            _run_android $2 $3
            ;;
        *)
            _abnr_usage
            ;;
    esac
}

function _abnr_usage
{
    echo "ABNR commandline Usage:                        "
    echo "                                               "
    echo "abnr  build  mac    host                       "
    echo "             inux   arm                        "
    echo "                    hvx128                     "
    echo "                    hvx64                      "
    echo "                                               "
    echo "abnr  run    mac      dark          <testname> "
    echo "             linux    bright                   "
    echo "             arm                               "
    echo "             hvx                               "
    echo "                                               "
    echo "abnr  pull   acuity3  dark                     "
    echo "             device   bright                   "
    echo "                                               "
    echo "abnr  convert  <src_folder>  <.raw>  packed    "
    echo "                                     mipi      "
}

function _build_tools
{
    cd ./tools/
    . build.sh
    cd ../
}

function _convert_raw
{
    if [ -z "$1" ]
    then
        echo "No arguments supplied"
        _abnr_usage
    else
        if [ "$3" = "packed" ]
        then
            _format="packed"
        else
            _format="mipi"
        fi

        for f in $1*$2
        do
            echo $f
            _dst_f=${f/$2/_unpacked.raw}
            ./tools/unpack $f 4032 3024 $_format $_dst_f 
            python tools/save_image.py -i $_dst_f  -w 4032 -h 3024
            rm $_dst_f
        done
    fi
    unset _format
}

function abnr
{
    case $1 in
        build)
            _clean
            _build_core $2 $3
            _build_lib $2 $3
            ;;
        run)
            _build_tools
            _run $2 $3 $4
            ;;
        convert)
            _convert_raw $2 $3 $4
            ;;
        pull)
            _pull $2 $3
            ;;    
        test)
            # input_dir out_dir cfg_file raw_format
            _generate_config_file $2 $3 
            ;;
        *|help)
            _abnr_usage
            ;;
    esac
}

