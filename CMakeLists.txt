
cmake_minimum_required(VERSION 2.8)

set(coreSrc
    base.cpp error.cpp md5sum.cpp string.cpp
)

add_library(core STATIC ${coreSrc})

