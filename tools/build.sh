# SRCS="split_image.cpp "
# COMPILER_OPTION="-std=c++11 -O3 -g -Wall -v"
# LINKER_OPTIONS="-ldl -lpthread -lz "
# OUT_TARGET="split "
# OUT_OPTION="-o $OUT_DIR$OUT_TARGET"
# CXX="g++"
#
# $CXX $COMPILER_OPTION $SRCS $OUT_OPTION $LINKER_OPTIONS

g++ unpack.cpp -std=c++11 -O3 -g -Wall -o unpack

g++ interp_g.cpp -std=c++11 -O3 -g -Wall -o interp_g 

g++ merge_rg.cpp -std=c++11 -O3 -g -Wall -o merge_rg
