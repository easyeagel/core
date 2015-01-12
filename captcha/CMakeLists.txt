
#编译captcha
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#===============================================================================================
set(captchaSrc
    captFontFile.cpp
    captGenerator.cpp
    captRLE.cpp
	)

add_library(captcha STATIC ${captchaSrc})
set_target_properties(captcha PROPERTIES
    COMPILE_FLAGS "${ComplierNoWaringFlags}")

