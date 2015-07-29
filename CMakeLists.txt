
cmake_minimum_required(VERSION 2.8)

set(coreSrc
    kvStore.cpp base.cpp error.cpp md5sum.cpp string.cpp server.cpp
    encode.cpp pipe.cpp io.cpp service.cpp sessionPool.cpp time.cpp
    log.cpp session.cpp daemon.cpp

    http/http.cpp
    http/http.hpp
    http/httpParser.cpp
    http/httpParser.hpp
    http/httpRoot.cpp
    http/httpRoot.hpp
    http/httpService.hpp
    http/httpSession.cpp
    http/httpSession.hpp
    http/webView.cpp
    http/webView.hpp
)

add_library(core STATIC ${coreSrc})

