cmake_minimum_required(VERSION 3.8)

set_source_files_properties(testfile.h testfile.c PROPERTIES COMPILE_FLAGS "${C_CXX_COMPILE_FLAGS} ${C_COMPILE_FLAGS}")

add_library(mytestlib STATIC testfile.c testfile.h)
target_include_directories(mytestlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

message(DEBUG "mytestlib link flags: ${MY_LINK_FLAGS}")
set_target_properties(mytestlib PROPERTIES
                      LINK_FLAGS ${MY_LINK_FLAGS}
                      )
target_link_libraries(mytestlib PUBLIC asf)
