
#编译snappy
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#===============================================================================================
set(snappySrc snappy.cc snappy-sinksource.cc snappy-stubs-internal.cc snappy-c.cc)

add_library(snappy STATIC ${snappySrc})
set_target_properties(snappy PROPERTIES
    COMPILE_FLAGS "${ComplierNoWaringFlags}"
    COMPILE_DEFINITIONS "HAVE_CONFIG_H")


