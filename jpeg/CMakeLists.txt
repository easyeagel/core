
cmake_minimum_required(VERSION 2.8)

set(jpegSrc
    jmemnobs.c jaricom.c jcapimin.c jcapistd.c jcarith.c jccoefct.c jccolor.c
    jcdctmgr.c jchuff.c jcinit.c jcmainct.c jcmarker.c jcmaster.c
    jcomapi.c  jcparam.c jcprepct.c jcsample.c jctrans.c jdapimin.c
    jdapistd.c jdarith.c jdatadst.c jdatasrc.c jdcoefct.c jdcolor.c
    jddctmgr.c jdhuff.c jdinput.c jdmainct.c jdmarker.c jdmaster.c
    jdmerge.c  jdpostct.c jdsample.c jdtrans.c jerror.c jfdctflt.c
    jfdctfst.c jfdctint.c jidctflt.c jidctfst.c jidctint.c jquant1.c
    jquant2.c  jutils.c jmemmgr.c
    cdjpeg.c rdcolmap.c rdswitch.c transupp.c rdppm.c wrppm.c rdgif.c
    wrgif.c rdtarga.c wrtarga.c rdbmp.c wrbmp.c rdrle.c wrrle.c
)

add_library(jpeg STATIC ${jpegSrc})
set_target_properties(jpeg PROPERTIES COMPILE_FLAGS "-w")

