#!/bin/bash

retVal=0

# $1 file name relative to src/ivy/graphics/shaders/
compile_shader() {
    glslc $1 -o ../../../../assets/shaders/$1.spv

    if [ $? -ne 0 ]; then
        retVal=1
    fi
}

# Make out directory
mkdir -p ./assets/shaders

# cd to source directory
cd src/ivy/graphics/shaders

# Compile shaders in current directory
for filename in *.{vert,frag,geom,comp}; do
    # Make sure the file exists
    [ -e $filename ] || continue

    # Print a status message and compile
    echo "Compiling $filename"
    compile_shader $filename
done

exit $retVal
