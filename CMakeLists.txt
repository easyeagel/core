
cmake_minimum_required(VERSION 2.8)

set(coreSrc
    kvStore.cpp base.cpp error.cpp md5sum.cpp string.cpp server.cpp
    encode.cpp pipe.cpp io.cpp service.cpp sessionPool.cpp time.cpp
    log.cpp session.cpp daemon.cpp
)

add_library(core STATIC ${coreSrc})

