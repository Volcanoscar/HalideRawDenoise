RED='\033[0;31m'
BLUE='\033[0;34m'
GREEN='\033[0;32m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

echo -e "\n${GREEN}==== Build Core ====${NC}"
echo -e "build flavor : ${PURPLE}$1${NC}" 
echo -e "build target : ${PURPLE}$2${NC}"

case $1 in
    mac) 
        HALIDE_ROOT="../../external/HalideHost"
        LIBRARY_OPTIONS="-L $HALIDE_ROOT/bin/ -lHalide"
        ABI=""
    ;;
    linux)
        HALIDE_ROOT="../../external/HalideQ"
        LIBRARY_OPTIONS="$HALIDE_ROOT/lib/libHalide.a "
        #LIBRARY_OPTIONS="-L $HALIDE_ROOT/lib/ -lHalide "
        ABI="-D _GLIBCXX_USE_CXX11_ABI=0"
    ;;
    *)
        echo -e "\n${RED}Wrong build flavor: $1${NC}"
        exit -2
    ;;
esac

case $2 in
    hvx64|hvx128|arm|win32|host)

    ;;
    *)
        echo -e "\n${RED} Wrong build target: $2${NC}"
        exit -2
    ;;        
esac

echo HALIDE_ROOT=$HALIDE_ROOT
echo LIBRARY_OPTIONS=$LIBRARY_OPTIONS


INCLUDES="-I $HALIDE_ROOT/include/ -I $HALIDE_ROOT/tools/ "

SRCS="nr_generator.cpp util.cpp merge.cpp align.cpp dct8.cpp $HALIDE_ROOT/tools/GenGen.cpp  "

COMPILER_OPTION="-std=c++11 -Wall -g -fno-rtti "
LINKER_OPTIONS="-ldl -lpthread -lz "
OUT_PARENT_DIR="build/"
OUT_TARGET="nr_gen "
OUT_OPTION="-o $OUT_PARENT_DIR$OUT_TARGET"
ABI="-D _GLIBCXX_USE_CXX11_ABI=0"
CXX="g++"

if [ ! -d "$OUT_PARENT_DIR" ]; then
  mkdir $OUT_PARENT_DIR
fi

${CXX} $COMPILER_OPTION $ABI $INCLUDES -g -fno-rtti $SRCS $LIBRARY_OPTIONS $OUT_OPTION $LINKER_OPTIONS
echo ${CXX} $COMPILER_OPTION $ABI $INCLUDES -g -fno-rtti $SRCS $LIBRARY_OPTIONS $OUT_OPTION $LINKER_OPTIONS

GEN_EXECUTABLE=$OUT_PARENT_DIR$OUT_TARGET
CORE_GEN_NAME="abnr_gen"
GEN_LIB="lib$CORE_GEN_NAME"

case $2 in
    hvx64)
        echo -e "\n${PURPLE}Build generator for HVX 64${NC}"
        HVX_64_DIR="hvx64/"
        OUT_GEN_DIR=$OUT_PARENT_DIR$HVX_64_DIR
        rm -rf $OUT_GEN_DIR
        mkdir -p $OUT_GEN_DIR
        echo $GEN_EXECUTABLE -g $CORE_GEN_NAME -f $GEN_LIB -o $OUT_GEN_DIR target=arm-64-android-hvx_64
        $GEN_EXECUTABLE -g $CORE_GEN_NAME -f $GEN_LIB -o $OUT_GEN_DIR target=arm-64-android-hvx_64
        EXTENSION=".a"
        ;;
    hvx128)
        echo -e "\n${PURPLE}Build generator for  HVX 128${NC}"
        HVX_128_DIR="hvx128/"
        OUT_GEN_DIR=$OUT_PARENT_DIR$HVX_128_DIR
        rm -rf $OUT_GEN_DIR
        mkdir -p $OUT_GEN_DIR
        echo $GEN_EXECUTABLE -g $CORE_GEN_NAME -f $GEN_LIB -o $OUT_GEN_DIR target=arm-64-android-hvx_128
        $GEN_EXECUTABLE -g $CORE_GEN_NAME -f $GEN_LIB -o $OUT_GEN_DIR target=arm-64-android-hvx_128
        EXTENSION=".a"
        ;;
    arm)
        echo -e "\n${PURPLE}Build generator for ARM${NC}"        
        ARM64_DIR="arm64/"
        OUT_GEN_DIR=$OUT_PARENT_DIR$ARM64_DIR
        rm -rf $OUT_GEN_DIR
        mkdir -p $OUT_GEN_DIR
        echo $GEN_EXECUTABLE -g $CORE_GEN_NAME -f $GEN_LIB -o $OUT_GEN_DIR target=arm-64-android        
        $GEN_EXECUTABLE -g $CORE_GEN_NAME -f $GEN_LIB -o $OUT_GEN_DIR target=arm-64-android
        EXTENSION=".a"
        ;;
    win32)
        WIN32_DIR="win32/"
        OUT_GEN_DIR=$OUT_PARENT_DIR$WIN32_DIR
        rm -rf $OUT_GEN_DIR
        mkdir $OUT_GEN_DIR
        $GEN_EXECUTABLE -g $CORE_GEN_NAME -f $GEN_LIB -o $OUT_GEN_DIR target=x86-32-windows
        EXTENSION=".lib"
        ;;
    host)
        echo -e "\n${PURPLE}Build generator for Host${NC}"   
        HOST_DIR="host/"
        OUT_GEN_DIR=$OUT_PARENT_DIR$HOST_DIR
        rm -rf $OUT_GEN_DIR 
        mkdir $OUT_GEN_DIR
        $GEN_EXECUTABLE -g $CORE_GEN_NAME -f $GEN_LIB -o $OUT_GEN_DIR target=host
        EXTENSION=".a"
        ;;
    *)
        echo -e "\n${RED} Wrong build target: $2${NC}"
        exit -2
    ;;        
esac

OUTPUT_DIR="../../build/"
if [ ! -d "$OUTPUT_DIR" ]; then
  mkdir $OUTPUT_DIR
fi
GEN_HEADER="$GEN_LIB.h"
GEN_LIB_FILE=$GEN_LIB$EXTENSION

cp $OUT_GEN_DIR$GEN_LIB_FILE $OUTPUT_DIR
cp $OUT_GEN_DIR$GEN_HEADER $OUTPUT_DIR
    
if [ $? -eq 0 ]; then
    echo -e "\n${BLUE}Build Core Successfully${NC}\n"
else
    echo -e "\n${RED}Build Core Failed${NC}\n"
fi