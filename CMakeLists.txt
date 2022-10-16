cmake_minimum_required(VERSION 3.8)
include(cmake_sources.cmake)
include(configs.cmake)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_HOME_DIRECTORY}/sam4e.cmake)
message(DEBUG "toolchain file is  ${CMAKE_TOOLCHAIN_FILE}")

set(SOURCES_DIRECTORY src)

project("ClampMeter" LANGUAGES C CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/archive")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

set(ARM_CPU cortex-m4)
set(MCU_TYPE SAM4E8C)
string(TOLOWER ${MCU_TYPE} MCU_TYPE_LOWERCASE)

set(ATMEL_ARCHITECTURE SAM4E)

set(EXECUTABLE_NAME ${PROJECT_NAME})

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(C_CXX_CONDITIONAL_COMPILE_FLAGS ${C_CXX_COMPILER_OPTIONS_DEBUG})
    set(CXX_CONDITIONAL_COMPILE_FLAGS ${CXX_COMPILER_OPTIONS_DEBUG})
else ()
    set(C_CXX_CONDITIONAL_COMPILE_FLAGS ${C_CXX_COMPILER_OPTIONS_RELEASE})
endif ()

set(C_CXX_COMPILE_FLAGS "${COMPILER_SHOW_HEADERS_CMD} ${C_CXX_CONDITIONAL_COMPILE_FLAGS} -mthumb -D__${MCU_TYPE}__  -DBOARD=USER_BOARD -DARM_MATH_CM4=true -Dprintf=iprintf -ffast-math --param max-inline-insns-single=500 -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffunction-sections -mlong-calls -g3 -Wall -mcpu=${ARM_CPU} -c")
set(C_COMPILE_FLAGS "-x c -std=gnu17")
set(CXX_COMPILE_FLAGS "${CXX_CONDITIONAL_COMPILE_FLAGS} -D__FPU_PRESENT -DARM_MATH_CM4=true -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -fconcepts -fconcepts-diagnostics-depth=2 -fno-exceptions -std=gnu++20")

#set(LINKER_SCRIPT_DIR "C:/Program Files (x86)/Atmel/Studio/7.0/packs/atmel/SAM4E_DFP/1.1.57/ic.sam4e/gcc/gcc")
set(LINKER_SCRIPT_DIR "${CMAKE_SOURCE_DIR}/src/asf_lib/src/ASF/sam/utils/linker_scripts/sam4e/sam4e8/gcc")
#set(LINKER_SCRIPT_FILENAME ${MCU_TYPE_LOWERCASE}_flash.ld)
set(LINKER_SCRIPT_FILENAME flash.ld)
set(LINKER_SCRIPT_COMMAND "-L\"${LINKER_SCRIPT_DIR}\" -T${LINKER_SCRIPT_FILENAME}")
set(LINKER_MAP_GEN_COMMAND "-Wl,-Map=\"${PROJECT_NAME}.map\"")

set(MY_LINK_FLAGS "${LINKER_VERBOSE_CMD} --specs=nosys.specs -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -mthumb -mcpu=cortex-m4 ${LINKER_MAP_GEN_COMMAND} -Wl,--start-group -lm -Wl,--end-group -Wl,--gc-sections ${LINKER_SCRIPT_COMMAND}")
message(DEBUG "link flags: ${MY_LINK_FLAGS}")

set(CXX_SOURCES
    ${SOURCES_DIRECTORY}/main.cpp
    ${SOURCES_DIRECTORY}/maintask.cpp
    ${SOURCES_DIRECTORY}/maintask.hpp
    ${SOURCES_DIRECTORY}/hclamp_meter.hpp
    ${SOURCES_DIRECTORY}/mutex/semaphore.hpp
    ${SOURCES_DIRECTORY}/mutex/semaphore.cpp
    ${SOURCES_DIRECTORY}/ili9486/ili9486_driver.cpp
    ${SOURCES_DIRECTORY}/ili9486/ili9486_driver.hpp
    ${SOURCES_DIRECTORY}/ili9486/font.hpp
    ${SOURCES_DIRECTORY}/clamp_sensor/clamp_sensor.cpp
    ${SOURCES_DIRECTORY}/clamp_sensor/clamp_sensor.hpp
    ${SOURCES_DIRECTORY}/clamp_meter_drawer/clamp_meter_drawing.cpp
    ${SOURCES_DIRECTORY}/clamp_meter_drawer/clamp_meter_drawing.hpp
    ${SOURCES_DIRECTORY}/main_task/main_task.hpp
    ${SOURCES_DIRECTORY}/main_task/main_task.cpp
    ${SOURCES_DIRECTORY}/clamp_meter_drawer/display_drawer.hpp
#    ${SOURCES_DIRECTORY}/keyboard/keyboard.hpp
#    ${SOURCES_DIRECTORY}/keyboard/keyboard.cpp
#    ${SOURCES_DIRECTORY}/keyboard/mcp23016_driver.hpp
#    ${SOURCES_DIRECTORY}/keyboard/mcp23016_driver.cpp
    ${SOURCES_DIRECTORY}/keyboard/pin.hpp
    ${SOURCES_DIRECTORY}/timer/timer.cpp
    ${SOURCES_DIRECTORY}/timer/timer.hpp
    ${SOURCES_DIRECTORY}/concepts/clamp_meter_concepts.hpp
    ${SOURCES_DIRECTORY}/keyboard_fast/keyboard_fast.hpp
    ${SOURCES_DIRECTORY}/keyboard_fast/mcp23016_driver_fast.hpp
    ${SOURCES_DIRECTORY}/keyboard_fast/mcp23016_driver_fast.cpp
    ${SOURCES_DIRECTORY}/keyboard_fast/button.cpp
    ${SOURCES_DIRECTORY}/keyboard_fast/button.hpp
    ${SOURCES_DIRECTORY}/keyboard_fast/encoder.hpp
    ${SOURCES_DIRECTORY}/keyboard_fast/encoder.cpp
    ${SOURCES_DIRECTORY}/menu/menu_model_item.cpp
    ${SOURCES_DIRECTORY}/menu/menu_model_item.hpp
    ${SOURCES_DIRECTORY}/menu/menu.hpp
    ${SOURCES_DIRECTORY}/menu/menu.cpp
    ${SOURCES_DIRECTORY}/menu/menu_model_drawer.hpp
    ${SOURCES_DIRECTORY}/menu/menu_model_drawer.cpp
    ${SOURCES_DIRECTORY}/menu/menu_model.hpp
    ${SOURCES_DIRECTORY}/utilities/universal_type.hpp
    ${SOURCES_DIRECTORY}/dsp/dsp.hpp
    )

set(C_SOURCES
    src/main_blinky.c
    src/main_blinky.h
    src/freertos_handlers.c
    src/exception_handlers.c
    src/main_task/cmaintask.c
    src/main_task/cmaintask.h
    )

set(INCLUDE_DIRECTORIES
    ${SOURCES_DIRECTORY}
    ${SOURCES_DIRECTORY}/clamp_meter_drawer
    ${SOURCES_DIRECTORY}/clamp_sensor
    ${SOURCES_DIRECTORY}/ili9486
    ${SOURCES_DIRECTORY}/mutex
    ${SOURCES_DIRECTORY}/main_task
    ${SOURCES_DIRECTORY}/keyboard
    ${SOURCES_DIRECTORY}/timer
    ${SOURCES_DIRECTORY}/concepts
    ${SOURCES_DIRECTORY}/keyboard_fast
    ${SOURCES_DIRECTORY}/menu
    ${SOURCES_DIRECTORY}/utilities
    ${SOURCES_DIRECTORY}/dsp
    )

set_source_files_properties(${CXX_SOURCES} PROPERTIES COMPILE_FLAGS "${C_CXX_COMPILE_FLAGS} ${CXX_COMPILE_FLAGS}")
set_source_files_properties(${C_SOURCES} PROPERTIES COMPILE_FLAGS "${C_CXX_COMPILE_FLAGS} ${C_COMPILE_FLAGS}")

add_sam_executable(${EXECUTABLE_NAME} ${CXX_SOURCES} ${C_SOURCES})
target_include_directories(${EXECUTABLE_NAME} PRIVATE
                           ${INCLUDE_DIRECTORIES}
                           )

add_subdirectory(src/asf_lib)
target_link_libraries(${EXECUTABLE_NAME} PUBLIC asf)

add_subdirectory(src/clamp_meter)
target_link_libraries(${EXECUTABLE_NAME} PUBLIC clamp_meter)

display_size(${EXECUTABLE_NAME})