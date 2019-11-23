#######################################################################
# use for compiling abnr prj, output will be present in $(BUILD_FOLDER)
# how to use:
# clean build: make clean
# compile build: make
#######################################################################


ifeq (Q, $(HALIDE))
  HALIDE_ROOT = external/HalideQ
else
  HALIDE_ROOT = /etc/halide
endif

BUILD_FOLDER = build
LD_LIB_PATH = LD_LIBRARY_PATH=$(HALIDE_ROOT)/bin
MKDIR_P = mkdir -p
CORE_GEN_NAME=abnr_gen
CORE_GEN_LIB = lib$(CORE_GEN_NAME)
core_folder = src/core/
lib_folder = src/lib/
app_exe = nr
GEN_OUT = $(BUILD_FOLDER)/nr_gen
GEN_GEN = $(HALIDE_ROOT)/tools/GenGen.cpp 
HALIDE_BIN = -lHalide -L $(HALIDE_ROOT)/bin 
HALIDE_ALIB = $(HALIDE_ROOT)/lib/libHalide.a
INCLUDES = -I $(HALIDE_ROOT)/include/ -I $(HALIDE_ROOT)/tools/ -I $(BUILD_FOLDER) 

#### SOURCE CODE ####
gen_src = $(core_folder)nr_generator.cpp $(core_folder)merge.cpp $(core_folder)align.cpp $(core_folder)dct8.cpp $(core_folder)util.cpp
app_src = $(lib_folder)ai_denoise.cpp $(lib_folder)main.cpp

####COMPILERS####
CC = g++ 
ANDROID_CC = /etc/arm64-android-toolchain/bin/aarch64-linux-android-g++
ANDROID_CC_QC=/home/acuity/Qualcomm/Hexagon_SDK/3.3.3/tools/android-ndk-r14b/toolchains/aarch64-linux-android-4.9/prebuilt/linux-x86_64/bin/aarch64-linux-android-c++
ANDROID_CC_LOC=external/Android_Arm64_toolchain/bin/aarch64-linux-android-g++

####FLAGS####
CXXFLAGS= -std=c++11 -g -fno-rtti 
LINK_LIB= -ldl -lpthread -lz
OPT_FLAG = -O3
NO_ABI = -D _GLIBCXX_USE_CXX11_ABI=0
MISC_FLAGS = -llog -fPIE -pie
MP_FLAG = -fopenmp

####TARGET FLAVOR####
TARGET_HVX128 = target=arm-64-android-hvx_128
TARGET_ARM = target=arm-64-android
TARGET_HOST = target=host

all:exe 

dir:clean
	${MKDIR_P} $(BUILD_FOLDER)
tool_gen:
	$(CC) tools/unpack.cpp -std=c++11 -O3 -g -Wall -o tools/unpack
	$(CC) tools/interp_g.cpp -std=c++11 -O3 -g -Wall -o tools/interp_g 
	$(CC) tools/merge_rg.cpp -std=c++11 -O3 -g -Wall -o tools/merge_rg
abnr_gen: dir 
	$(CC) $(gen_src) $(GEN_GEN) $(CXXFLAGS) $(INCLUDES) $(HALIDE_BIN) -o $(GEN_OUT)
abnr_lib:abnr_gen
	$(LD_LIB_PATH) ./$(GEN_OUT) -g $(CORE_GEN_NAME) -f $(CORE_GEN_LIB) -o $(BUILD_FOLDER) $(TARGET_HOST)
exe:abnr_lib tool_gen
	$(CC) $(app_src) $(BUILD_FOLDER)/$(CORE_GEN_LIB).a $(CXXFLAGS) $(LINK_LIB) $(INCLUDES) $(HALIDE_BIN) -o $(BUILD_FOLDER)/$(app_exe) 


abnr_gen_arm:dir
	$(CC) $(CXXFLAGS) $(NO_ABI) $(INCLUDES) $(GEN_GEN) $(gen_src) $(HALIDE_ALIB) -o $(GEN_OUT) $(LINK_LIB)

abnr_lib_arm:abnr_gen_arm
	$(LD_LIB_PATH) ./$(GEN_OUT) -g $(CORE_GEN_NAME) -f $(CORE_GEN_LIB) -o $(BUILD_FOLDER) $(TARGET_ARM)

exe_arm:abnr_lib_arm
	$(ANDROID_CC_QC) $(app_src) $(BUILD_FOLDER)/$(CORE_GEN_LIB).a $(CXXFLAGS) $(INCLUDES) $(HALIDE_BIN) -o $(BUILD_FOLDER)/$(app_exe) $(MISC_FLAGS) 

exe_arm_a:abnr_lib_arm
	$(ANDROID_CC_LOC) $(CXXFLAGS) $(INCLUDES) $(app_src) $(HALIDE_ALIB) $(BUILD_FOLDER)/$(CORE_GEN_LIB).a -o $(BUILD_FOLDER)/$(app_exe) $(MISC_FLAGS) 

android: exe_arm_a

.PHONY: clean
clean:
	rm -rf $(BUILD_FOLDER)   
