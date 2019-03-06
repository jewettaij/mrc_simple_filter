#!/bin/sh

export ANSI_C="gcc"
export ANSI_CPP="g++"
export L_COMP="ar rs"

export LFLAGS=""
# (Note: Static linking is recommended, but I could not get it to
#        work on the mac version of the gcc compiler (as of 2008))

# Note: I get strange behavior ("Heisenbug") if I use optimizations (-O3)
# COMMENTING OUT:
# export MY_FLAGS="-std=c++17 -O3 -DNDEBUG -ffast-math -DDISABLE_OPENMP"
# INSTEAD USE:
export MY_FLAGS="-std=c++17 -DNDEBUG -ffast-math -DDISABLE_OPENMP"
export CFLAGS="-c $MY_FLAGS"
export CPP_PRELINKER_COMMAND="echo"
export COMPILER_TEMP_FILES=""
export LINKER_TEMP_FILES=""
