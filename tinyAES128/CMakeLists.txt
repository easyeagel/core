
#编译tinyAES128
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#===============================================================================================
set(tinyAES128Src aes.c aes.h)
add_library(tinyAES128 STATIC ${tinyAES128Src})
set_target_properties(tinyAES128 PROPERTIES
    COMPILE_FLAGS "${ComplierNoWaringFlags}")
target_include_directories(tinyAES128
    PUBLIC "${CMAKE_CUREENT_DIR}"
)
