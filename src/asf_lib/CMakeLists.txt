cmake_minimum_required(VERSION 3.8)
set(LIB_NAME asf)
set(MATH_LIB_NAME "libarm_cortexM4lf_math_softfp.a")

include(sources.cmake)

set(LIB_COMPILE_FLAGS "${C_CXX_COMPILE_FLAGS} ${C_COMPILE_FLAGS}")
string(APPEND LIB_COMPILE_FLAGS " -DBOARD_FREQ_SLCK_XTAL=32768UL -DBOARD_FREQ_SLCK_BYPASS=32768UL -DBOARD_FREQ_MAINCK_XTAL=20000000UL -DBOARD_FREQ_MAINCK_BYPASS=20000000UL -DBOARD_OSC_STARTUP_US=15625UL")

#set(LINKER_SCRIPT_DIR "C:/Program Files (x86)/Atmel/Studio/7.0/packs/atmel/SAM4E_DFP/1.1.57/ic.sam4e/gcc/gcc")
#set(LINKER_SCRIPT_FILENAME ${MCU_TYPE_LOWERCASE}_flash.ld)
#set(LINKER_SCRIPT_COMMAND "-L\"${LINKER_SCRIPT_DIR}\" -T${LINKER_SCRIPT_FILENAME}")
#set(LINKER_MAP_GEN_COMMAND "-Wl,-Map=\"${LIB_NAME}.map\"")
#set(LIB_LINK_FLAGS "-mthumb -mcpu=cortex-m4 ${LINKER_MAP_GEN_COMMAND} -Wl,--start-group -lm -Wl,--end-group -Wl,--gc-sections ${LINKER_SCRIPT_COMMAND}")
#message(DEBUG "library ${LIB_NAME} link flags:: ${MY_LINK_FLAGS}")

set_source_files_properties(${ASF_SOURCES} ${FREERTOS_SOURCES} PROPERTIES
                            COMPILE_FLAGS ${LIB_COMPILE_FLAGS})

add_library(${LIB_NAME} OBJECT ${ASF_SOURCES} ${FREERTOS_SOURCES}) # OBJECT type of library is very important here

file(GLOB_RECURSE MATH_LIB_PATH ${MATH_LIB_NAME})
if ("${MATH_LIB_PATH}" STREQUAL "")
    message(FATAL_ERROR "Arm Math library <<< ${MATH_LIB_NAME} >>> not found")
endif()

target_link_libraries(${LIB_NAME} ${MATH_LIB_PATH})

set_target_properties(${LIB_NAME} PROPERTIES
                      LINK_FLAGS ${MY_LINK_FLAGS}
#                      LINKER_LANGUAGE "C"
                      )

target_include_directories(${LIB_NAME}
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                           PUBLIC ${ASF_INCLUDE_DIRECTORIES}
                           PUBLIC ${FREERTOS_INCLUDE_DIRS}
                           PUBLIC "${PACKS_REPO}/atmel/${ATMEL_ARCHITECTURE}_DFP/1.1.57/ic.sam4e/include"
                           PUBLIC "${PACKS_REPO}/arm/CMSIS/5.4.0/CMSIS/Core/Include"
                           )
