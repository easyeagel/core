
#编译lua
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#===============================================================================================
set(bzip2Src blocksort.c huffman.c crctable.c randtable.c compress.c decompress.c bzlib.c)

add_library(bzip2 STATIC ${bzip2Src})
set_target_properties(bzip2 PROPERTIES
    COMPILE_FLAGS "${ComplierNoWaringFlags}")


