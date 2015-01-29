

#编译sqlite
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#===============================================================================================
add_definitions(-DWEBP_HAVE_GL)
add_definitions(-DWEBP_HAVE_PNG)
add_definitions(-DWEBP_HAVE_JPEG)
add_definitions(-DWEBP_USE_THREAD)
if(OptAndroid)
    add_definitions(-DWEBP_USE_NEON)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=neon")
endif()
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(libWebpSrc 
    src/dec/alpha.c 
    src/dec/buffer.c 
    src/dec/frame.c 
    src/dec/idec.c 
    src/dec/io.c 
    src/dec/quant.c 
    src/dec/tree.c 
    src/dec/vp8.c 
    src/dec/vp8l.c 
    src/dec/webp.c 

    src/demux/demux.c 

    src/dsp/alpha_processing.c 
    src/dsp/alpha_processing_sse2.c 
    src/dsp/cpu.c 
    src/dsp/dec.c 
    src/dsp/dec_clip_tables.c 
    src/dsp/dec_mips32.c 
    src/dsp/dec_neon.c 
    src/dsp/dec_sse2.c 
    src/dsp/lossless.c 
    src/dsp/lossless_mips32.c 
    src/dsp/lossless_neon.c 
    src/dsp/lossless_sse2.c 
    src/dsp/upsampling.c 
    src/dsp/upsampling_neon.c 
    src/dsp/upsampling_sse2.c 
    src/dsp/yuv.c 
    src/dsp/yuv_mips32.c 
    src/dsp/yuv_sse2.c 

    src/dsp/enc.c 
    src/dsp/enc_avx2.c 
    src/dsp/enc_mips32.c 
    src/dsp/enc_neon.c 
    src/dsp/enc_sse2.c 

    src/enc/alpha.c 
    src/enc/analysis.c 
    src/enc/backward_references.c 
    src/enc/config.c 
    src/enc/cost.c 
    src/enc/filter.c 
    src/enc/frame.c 
    src/enc/histogram.c 
    src/enc/iterator.c 
    src/enc/picture.c 
    src/enc/picture_csp.c 
    src/enc/picture_psnr.c 
    src/enc/picture_rescale.c 
    src/enc/picture_tools.c 
    src/enc/quant.c 
    src/enc/syntax.c 
    src/enc/token.c 
    src/enc/tree.c 
    src/enc/vp8l.c 
    src/enc/webpenc.c 

    examples/jpegdec.c 
    examples/metadata.c 
    examples/pngdec.c 
    examples/tiffdec.c 
    examples/webpdec.c 

    examples/example_util.c 

    examples/gif2webp_util.c 

    src/mux/muxedit.c 
    src/mux/muxinternal.c 
    src/mux/muxread.c 

    src/utils/bit_reader.c 
    src/utils/color_cache.c 
    src/utils/filters.c 
    src/utils/huffman.c 
    src/utils/quant_levels_dec.c 
    src/utils/random.c 
    src/utils/rescaler.c 
    src/utils/thread.c 
    src/utils/utils.c 

    src/utils/bit_writer.c 
    src/utils/huffman_encode.c 
    src/utils/quant_levels.c 
)
add_library(webp STATIC ${libWebpSrc} examples/cwebp.c examples/dwebp.c)
if(OptLinux)
    set(PlatformLibs pthread m)
elseif(OptWindows)
   set(PlatformLibs )
elseif(OptAndroid)
   set(PlatformLibs cpufeatures)
endif()
target_link_libraries(webp png jpeg zlibstatic ${PlatformLibs})
set_target_properties(webp PROPERTIES
    INCLUDE_DIRECTORIES "${CoreDir}/webp/src;${CoreDir}/jpeg;${CoreDir}/png;${CMAKE_CONFIGURATION_HEADER_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    COMPILE_FLAGS "-w -DGMacroCWepbMain=cwebp_main -DGMacroDWebpMain=dwebp_main"
)

