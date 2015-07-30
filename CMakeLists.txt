
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
    http/mutilpart.cpp
    http/mutilpart.hpp
)

add_library(core STATIC ${coreSrc})

set(BoostLibs
    boost_regex  boost_program_options boost_filesystem boost_log
    boost_system boost_thread boost_coroutine boost_context
    boost_iostreams boost_date_time
)

if(OptWindows)
    set(BoostLibs ${BoostLibs} boost_bzip2 boost_zlib)
    set(dsub "-vc120-mt-sgd-1_58")
    set(rsub "-vc120-mt-s-1_58")

    target_link_libraries(core debug uafxcwd optimized uafxcw)
    target_link_libraries(core debug libcpmtd optimized libcpmt)
    foreach(file ${BoostLibs})
        target_link_libraries(core debug lib${file}${dsub} optimized lib${file}${rsub})
    endforeach()

elseif(OptLinux)
    set(sysLibs crypto bzip2 z m)
    target_link_libraries(core ${refLibs} ${BoostLibs} ${sysLibs} pthread dl)
endif()


