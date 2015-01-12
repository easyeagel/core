

#编译sqlite
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#===============================================================================================
set(sqliteSrc Column.cpp Database.cpp Statement.cpp Transaction.cpp sqlite3secure.c)
add_library(sqlite STATIC ${sqliteSrc})
set_target_properties(sqlite PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  COMPILE_FLAGS "-w -DSQLITE_USER_AUTHENTICATION=1 -DSQLITE_HAS_CODEC=1"
)


