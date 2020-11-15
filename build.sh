#!/bin/bash

# Style
./style.sh

# Prepare out directory
mkdir -p out
cd out

# Generate makefiles and build
cmake -G "Unix Makefiles" ../
make
