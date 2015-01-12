

#编译json
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#===============================================================================================
set(jsonSrc 
	_internal/Source/internalJSONNode.cpp _internal/Source/JSONAllocator.cpp _internal/Source/JSONChildren.cpp
	_internal/Source/JSONDebug.cpp _internal/Source/JSONIterators.cpp _internal/Source/JSONMemory.cpp
	_internal/Source/JSONNode.cpp _internal/Source/JSONNode_Mutex.cpp _internal/Source/JSONPreparse.cpp
	_internal/Source/JSONStream.cpp _internal/Source/JSONValidator.cpp _internal/Source/JSONWorker.cpp
	_internal/Source/JSONWriter.cpp _internal/Source/libjson.cpp)

add_library(json STATIC ${jsonSrc})
set_target_properties(json PROPERTIES
    COMPILE_FLAGS "${ComplierNoWaringFlags}"
    COMPILE_DEFINITIONS_DEBUG "JSON_SAFE;JSON_DEBUG;JSON_ISO_STRICT"
    COMPILE_DEFINITIONS_RELEASE "JSON_ISO_STRICT")


