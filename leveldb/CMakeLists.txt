

#--------------------------------------------------------------------------------------#
#平台相关的编译选项
if(MSVC)
    add_definitions(-DLEVELDB_PLATFORM_WINDOWS)
    add_definitions(-DOS_WIN)
    set(leveldbSrc
        db/builder.cc db/c.cc db/dbformat.cc db/db_impl.cc db/db_iter.cc db/filename.cc
	db/log_reader.cc db/log_writer.cc db/memtable.cc db/repair.cc db/table_cache.cc
	db/version_edit.cc db/version_set.cc db/write_batch.cc table/block_builder.cc
	table/block.cc table/filter_block.cc table/format.cc table/iterator.cc table/merger.cc
	table/table_builder.cc table/table.cc table/two_level_iterator.cc util/arena.cc util/bloom.cc
	util/cache.cc util/coding.cc util/comparator.cc util/crc32c.cc util/env.cc 
	util/filter_policy.cc util/hash.cc util/histogram.cc util/logging.cc util/options.cc
	util/status.cc)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_definitions(-DLEVELDB_PLATFORM_POSIX)
    add_definitions(-DOS_LINUX)
    set(leveldbSrc 
        table/iterator.cc table/filter_block.cc table/block.cc table/two_level_iterator.cc
        table/format.cc table/block_builder.cc table/merger.cc table/table.cc table/table_builder.cc
        util/coding.cc util/options.cc util/bloom.cc util/crc32c.cc util/histogram.cc util/testutil.cc
        util/comparator.cc util/arena.cc util/env.cc util/testharness.cc util/logging.cc
        util/status.cc util/filter_policy.cc util/cache.cc util/hash.cc helpers/memenv/memenv.cc
        db/builder.cc db/repair.cc db/db_impl.cc db/write_batch.cc db/c.cc db/filename.cc
        db/version_edit.cc db/leveldb_main.cc db/memtable.cc db/log_writer.cc db/version_set.cc
        db/dbformat.cc db/db_iter.cc db/table_cache.cc db/log_reader.cc util/env_posix.cc
        port/port_posix.cc
    )
endif()




#编译leveldb
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#===============================================================================================
add_library(leveldb STATIC ${leveldbSrc})
set_target_properties(leveldb PROPERTIES
    INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/include;${CMAKE_CURRENT_SOURCE_DIR}/../snappy"
    COMPILE_FLAGS "${ComplierNoWaringFlags}"
    COMPILE_DEFINITIONS "SNAPPY")


