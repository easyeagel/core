
cmake_minimum_required(VERSION 2.8)

set(coreSrc
    base.cpp error.cpp md5sum.cpp string.cpp server.cpp encode.cpp pipe.cpp
    io.cpp service.hpp sessionPool.cpp time.cpp log.cpp session.cpp
)

add_library(core STATIC ${coreSrc})

