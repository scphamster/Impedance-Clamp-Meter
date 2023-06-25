##################### CMAKE CONFIGS ####################
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_EXECUTABLE_SUFFIX_C .elf)
set(CMAKE_EXECUTABLE_SUFFIX_CXX .elf)
set(CMAKE_EXECUTABLE_SUFFIX_ASM .elf)

#set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
#set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(PACKS_REPO "/home/scphamster/dev/libraries/microchip/packs")

message(DEBUG " use external tools? ${USE_EXTERNAL_TOOLSET} :: toolsdir: ${TOOLSDIR}")
if (USE_EXTERNAL_TOOLSET)
    set(TOOLSDIR "C:/dev/tools/arm_gcc_toolchain/10 2021.10/bin")
else ()
    set(TOOLSDIR "C:/Program Files (x86)/Atmel/Studio/7.0/toolchain/arm/arm-gnu-toolchain/bin")
endif ()

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

set(CMAKE_C_COMPILER /usr/bin/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/arm-none-eabi-g++)

#find_program(CMAKE_C_COMPILER NAMES arm-none-eabi-gcc HINTS ${TOOLSDIR})
#find_program(CMAKE_CXX_COMPILER NAMES arm-none-eabi-g++ HINTS ${TOOLSDIR})
#find_program(CMAKE_OBJCOPY NAMES arm-none-eabi-objcopy HINTS ${TOOLSDIR})
#message(DEBUG "Use compiler executable as linker? ${USE_COMPILER_EXECUTABLE_AS_LINKER}")
#if (USE_COMPILER_EXECUTABLE_AS_LINKER)
#    find_program(CMAKE_LINKER NAMES arm-none-eabi-g++ HINTS ${TOOLSDIR})
#else ()
#    find_program(CMAKE_LINKER NAMES arm-none-eabi-ld HINTS ${TOOLSDIR})
#endif ()
#find_program(CMAKE_SIZE NAMES arm-none-eabi-size HINTS ${TOOLSDIR})

if (NOT ARM_CPU)
    set(
            ARM_CPU cortex-m4
            CACHE STRING "setdefault MCU: cortex-m4 (see 'arm-none-eabi-gcc --target-help' for valid values)"
    )
endif (NOT ARM_CPU)

if (NOT ATMEL_ARCHITECTURE)
    set(
            ATMEL_ARCHITECTURE SAM4E
            CACHE STRING "set the default architecture: SAM4E"
    )
endif (NOT ATMEL_ARCHITECTURE)

if (NOT MCU_TYPE)
    set(
            MCU_TYPE SAM4E8C
            CACHE STRING "setthe default MCU platform: sam4e8c"
    )
endif (NOT MCU_TYPE)

function(display_size TARGET)
    add_custom_target(${TARGET}_display_size ALL
                      COMMAND ${CMAKE_SIZE} ${TARGET}${CMAKE_EXECUTABLE_SUFFIX_C}
                      COMMENT "Target Size:"
                      DEPENDS ${TARGET}${CMAKE_EXECUTABLE_SUFFIX_C}
                      WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                      )
endfunction(display_size)

function(generate_hex_file FORTARGET)
    add_custom_command(TARGET ${FORTARGET} POST_BUILD
                       COMMAND ${CMAKE_OBJCOPY} -O ihex ${FORTARGET}${CMAKE_EXECUTABLE_SUFFIX_C} ${FORTARGET}.hex
                       COMMENT "Generating hex file ${FORTARGET}.hex"
                       DEPENDS ${TARGET}${CMAKE_EXECUTABLE_SUFFIX_C}
                       WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                       )
endfunction(generate_hex_file)

function(generate_binary_file MYTARGET)
    add_custom_command(TARGET ${MYTARGET} POST_BUILD
                       COMMAND ${CMAKE_OBJCOPY} -O binary ${MYTARGET}${CMAKE_EXECUTABLE_SUFFIX_C} ${MYTARGET}.bin
                       DEPENDS ${MYTARGET}${CMAKE_EXECUTABLE_SUFFIX_C}
                       WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                       COMMENT "Generating binary file" ${MYTARGET}.bin
                       )
endfunction(generate_binary_file)

function(add_sam_executable EXECUTABLE_NAME)
    set(SOURCES ${ARGN})
    list(LENGTH SOURCES num_of_source_files)

    if (num_of_source_files LESS 1)
        message(FATAL_ERROR "No source files provided for ${EXECUTABLE_NAME}")
    endif ()

    set(ELF_OUTPUT_FILE "${EXECUTABLE_NAME}.elf")
    set(BIN_OUTPUT_FILE "${EXECUTABLE_NAME}.bin")
    set(UF2_OUTPUT_FILE "${EXECUTABLE_NAME}.uf2")
    set(MAP_OUTPUT_FILE "${EXECUTABLE_NAME}.map")

    add_executable(${EXECUTABLE_NAME} ${SOURCES})

    set_target_properties(${EXECUTABLE_NAME}
                          PROPERTIES
                          LINK_FLAGS ${MY_LINK_FLAGS})

    target_link_libraries(${EXECUTABLE_NAME} ${MY_LIBS})

    get_directory_property(
            clean_files ADDITIONAL_MAKE_CLEAN_FILES
    )

#    set_directory_properties(
#            PROPERTIES
#            ADDITIONAL_MAKE_CLEAN_FILES "${MAP_OUTPUT_FILE}"
#    )
endfunction(add_sam_executable)

function(add_sam_library LIBRARY_NAME)
    set(additional_source_files ${ARGN})
    list(LENGTH additional_source_files num_of_source_files)

    message("starting include procedure")

    if (num_of_source_files LESS 0)
        # message(FATAL_ERROR "No source files provided for ${LIBRARY_NAME}")
    else ()
        foreach (src_file ${additional_source_files})
            # message(STATUS "Including source: ${src_file}")
        endforeach ()
    endif ()

    set(STATICLIB_OUTPUT_FILE "${LIBRARY_NAME}.a")

    # Create static library.
    add_library(${LIBRARY_NAME} STATIC ${additional_source_files})

    set_target_properties(
            ${LIBRARY_NAME}
            PROPERTIES
            COMPILE_FLAGS "-x c -mthumb -DDEBUG -D__${MCU_TYPE}__ -Og -ffunction-sections -mlong-calls -g3 -Wall -mcpu=${ARM_CPU}  -std=gnu11 -MD -MP -MF \" ${LIBRARY_NAME}.d\"" # -MT/"library.d/" -MT/"library.o/"   -o /"library.o/" /".././library.c/""
            LINKER_LANGUAGE "C"
            ARCHIVE_OUTPUT_NAME "${LIBRARY_NAME}"
    )
endfunction(add_sam_library)

function(add_my_library LIBRARY_NAME COMPILE_FLAGS LINK_FLAGS)
    set(LIB_FILES ${ARGN})
    list(LENGTH LIB_FILES NUMBER_OF_LIB_FILES)

    if (NUMBER_OF_LIB_FILES LESS 1)
        message(FATAL_ERROR "No sources specified for library ${LIBRARY_NAME}")
    endif ()

    set_source_files_properties(${LIB_FILES} PROPERTIES COMPILE_FLAGS ${COMPILE_FLAGS})
    add_library(${LIBRARY_NAME} STATIC ${LIB_FILES})

    target_link_libraries(${LIBRARY_NAME} ${${LIBRARY_NAME}_LIBS})
    set_target_properties(${LIBRARY_NAME} PROPERTIES
                          LINK_FLAGS ${LINK_FLAGS}
                          ARCHIVE_OUTPUT_NAME ${LIBRARY_NAME})

    target_include_directories(${LIBRARY_NAME}
                               PUBLIC ${${LIBRARY_NAME}_INCLUDE_DIRS}
                               )


endfunction()