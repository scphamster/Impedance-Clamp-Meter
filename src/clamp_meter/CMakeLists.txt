cmake_minimum_required(VERSION 3.8)
include(sources.cmake)

set(LIB_NAME clamp_meter)

set(LIB_COMPILE_FLAGS "${C_CXX_COMPILE_FLAGS} ${C_COMPILE_FLAGS}")
string(APPEND LIB_COMPILE_FLAGS " -DBOARD_FREQ_SLCK_XTAL=32768UL -DBOARD_FREQ_SLCK_BYPASS=32768UL -DBOARD_FREQ_MAINCK_XTAL=20000000UL -DBOARD_FREQ_MAINCK_BYPASS=20000000UL -DBOARD_OSC_STARTUP_US=15625UL")

set_source_files_properties(${SOURCES} PROPERTIES
                            COMPILE_FLAGS ${LIB_COMPILE_FLAGS})

add_library(${LIB_NAME} STATIC ${SOURCES})

target_link_libraries(${LIB_NAME} PUBLIC asf)

set_target_properties(${LIB_NAME} PROPERTIES
                      LINK_FLAGS ${MY_LINK_FLAGS}
                      )

target_include_directories(${LIB_NAME}
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
#                           PUBLIC ${INCLUDE_DIRECTORIES}
                           #PUBLIC "${PACKS_REPO}/atmel/${ATMEL_ARCHITECTURE}_DFP/1.1.57/ic.sam4e/include"
                           #PUBLIC "${PACKS_REPO}/arm/CMSIS/5.4.0/CMSIS/Core/Include"
                           )
