cmake_minimum_required(VERSION 3.8)
############   INCLUDES   #################
include(cmake_sources.cmake)
include(configs.cmake)

############### PROJECT ###################################
project("ClampMeter" LANGUAGES C CXX)
set(EXECUTABLE_NAME ${PROJECT_NAME})

########################## SOURCES #########################
set(CXX_SOURCES
    ${SOURCES_DIRECTORY}/compiler_compatibility_workaround.hpp
    ${SOURCES_DIRECTORY}/main.cpp

    ${SOURCES_DIRECTORY}/project_configs.hpp
    ${SOURCES_DIRECTORY}/tasks_controller.hpp

    ${SOURCES_DIRECTORY}/clamp_meter_driver.hpp

    ${SOURCES_DIRECTORY}/mutex/semaphore.hpp
    ${SOURCES_DIRECTORY}/mutex/semaphore.cpp

    ${SOURCES_DIRECTORY}/nvm_controller/flash_controller.hpp

    ${SOURCES_DIRECTORY}/ili9486/ili9486_driver.cpp
    ${SOURCES_DIRECTORY}/ili9486/ili9486_driver.hpp
    ${SOURCES_DIRECTORY}/ili9486/font.hpp
    ${SOURCES_DIRECTORY}/clamp_meter_drawer/clamp_meter_drawing.cpp
    ${SOURCES_DIRECTORY}/clamp_meter_drawer/clamp_meter_drawing.hpp
    ${SOURCES_DIRECTORY}/main_task/main_task.hpp
    ${SOURCES_DIRECTORY}/main_task/main_task.cpp
    ${SOURCES_DIRECTORY}/clamp_meter_drawer/display_drawer.hpp
    #    ${SOURCES_DIRECTORY}/keyboard/keyboard.hpp
    #    ${SOURCES_DIRECTORY}/keyboard/keyboard.cpp
    #    ${SOURCES_DIRECTORY}/keyboard/mcp23016_driver.hpp
    #    ${SOURCES_DIRECTORY}/keyboard/mcp23016_driver.cpp
    ${SOURCES_DIRECTORY}/timer/timer.cpp
    ${SOURCES_DIRECTORY}/timer/timer.hpp
    ${SOURCES_DIRECTORY}/concepts/clamp_meter_concepts.hpp

    ${SOURCES_DIRECTORY}/keyboard/pin.hpp
    ${SOURCES_DIRECTORY}/keyboard_fast/keyboard_fast.hpp
    ${SOURCES_DIRECTORY}/keyboard_fast/mcp23016_driver_fast.hpp
    ${SOURCES_DIRECTORY}/keyboard_fast/mcp23016_driver_fast.cpp
    ${SOURCES_DIRECTORY}/keyboard_fast/button.cpp
    ${SOURCES_DIRECTORY}/keyboard_fast/button.hpp
    #    ${SOURCES_DIRECTORY}/keyboard_fast/encoder.hpp
    #    ${SOURCES_DIRECTORY}/keyboard_fast/encoder.cpp

    ${SOURCES_DIRECTORY}/menu/menu_model.hpp
    ${SOURCES_DIRECTORY}/menu/menu_model_item.hpp
    ${SOURCES_DIRECTORY}/menu/menu_model_drawer.hpp
    ${SOURCES_DIRECTORY}/menu/menu_model_drawer.cpp
    ${SOURCES_DIRECTORY}/menu/menu_model_dialog.hpp

    ${SOURCES_DIRECTORY}/utilities/universal_type.hpp
    ${SOURCES_DIRECTORY}/utilities/string_converter.hpp

    ${SOURCES_DIRECTORY}/analog/adc/mcp3462_driver.cpp
    ${SOURCES_DIRECTORY}/analog/adc/mcp3462_driver.hpp
    ${SOURCES_DIRECTORY}/analog/dsp/filter.hpp
    ${SOURCES_DIRECTORY}/analog/dsp/filter.cpp
    ${SOURCES_DIRECTORY}/analog/dsp/sensor.cpp
    ${SOURCES_DIRECTORY}/analog/dsp/sensor.hpp
    ${SOURCES_DIRECTORY}/analog/dsp/dsp_resources.hpp
    ${SOURCES_DIRECTORY}/analog/dsp/dsp_resources.cpp
    ${SOURCES_DIRECTORY}/analog/dsp/iq_calculator.hpp
    ${SOURCES_DIRECTORY}/analog/dsp/deviation_calculator.hpp

    ${SOURCES_DIRECTORY}/analog/sensor/sensor_preamp.cpp
    ${SOURCES_DIRECTORY}/analog/sensor/sensor_preamp.hpp
    ${SOURCES_DIRECTORY}/analog/generator/output_generator.hpp
    ${SOURCES_DIRECTORY}/analog/generator/output_generator.cpp

    ${SOURCES_DIRECTORY}/peripherals/abstract_peripheral.hpp
    ${SOURCES_DIRECTORY}/peripherals/abstract_peripheral.cpp
    ${SOURCES_DIRECTORY}/peripherals/HVPowerSupply.hpp
    ${SOURCES_DIRECTORY}/peripherals/HVPowerSupply.cpp
    ${SOURCES_DIRECTORY}/peripherals/OutputRelay.cpp
    ${SOURCES_DIRECTORY}/peripherals/OutputRelay.hpp

    ${SOURCES_DIRECTORY}/peripherals/discrete_drivers/DG442/DG442.hpp
    ${SOURCES_DIRECTORY}/peripherals/discrete_drivers/DG442/DG442.cpp

    ${SOURCES_DIRECTORY}/peripherals/discrete_drivers/buzzer/buzzer.cpp
    ${SOURCES_DIRECTORY}/peripherals/discrete_drivers/buzzer/buzzer.hpp
    ${SOURCES_DIRECTORY}/peripherals/discrete_drivers/buzzer/buzzer_with_task.hpp

    ${SOURCES_DIRECTORY}/peripherals/analog_switch/analog_switch.hpp
    ${SOURCES_DIRECTORY}/peripherals/gain_control/amplifier_controller.hpp
    ${SOURCES_DIRECTORY}/peripherals/gain_control/automatic_gain_controller.hpp
    ${SOURCES_DIRECTORY}/peripherals/gain_control/gain_controller.hpp

    ${SOURCES_DIRECTORY}/freertos_cpp_wrappers/task/task.cpp
    ${SOURCES_DIRECTORY}/freertos_cpp_wrappers/task/task.hpp

    ${SOURCES_DIRECTORY}/freertos_cpp_wrappers/queue/queue.hpp
    ${SOURCES_DIRECTORY}/freertos_cpp_wrappers/queue/queue.cpp

    ${SOURCES_DIRECTORY}/freertos_cpp_wrappers/semaphore/semaphore.hpp

    ${SOURCES_DIRECTORY}/tests/agc_tests.hpp
    ${SOURCES_DIRECTORY}/tests/amplifier_controller_tests.hpp
    )

set(C_SOURCES
    src/freertos_handlers.c
    src/exception_handlers.c
    )

set(INCLUDE_DIRECTORIES
    ${SOURCES_DIRECTORY}
    ${SOURCES_DIRECTORY}/clamp_meter_drawer
    ${SOURCES_DIRECTORY}/ili9486
    ${SOURCES_DIRECTORY}/mutex
    ${SOURCES_DIRECTORY}/nvm_controller
    ${SOURCES_DIRECTORY}/main_task
    ${SOURCES_DIRECTORY}/keyboard
    ${SOURCES_DIRECTORY}/timer
    ${SOURCES_DIRECTORY}/concepts
    ${SOURCES_DIRECTORY}/keyboard_fast
    ${SOURCES_DIRECTORY}/menu
    ${SOURCES_DIRECTORY}/utilities
    ${SOURCES_DIRECTORY}/analog/adc
    ${SOURCES_DIRECTORY}/analog/dsp
    ${SOURCES_DIRECTORY}/analog/generator
    ${SOURCES_DIRECTORY}/analog/sensor

    ${SOURCES_DIRECTORY}/peripherals
    ${SOURCES_DIRECTORY}/peripherals/discrete_drivers/DG442

    ${SOURCES_DIRECTORY}/peripherals/discrete_drivers/buzzer

    ${SOURCES_DIRECTORY}/peripherals/analog_switch
    ${SOURCES_DIRECTORY}/peripherals/gain_control

    ${SOURCES_DIRECTORY}/freertos_cpp_wrappers
    ${SOURCES_DIRECTORY}/freertos_cpp_wrappers/task
    ${SOURCES_DIRECTORY}/freertos_cpp_wrappers/queue
    ${SOURCES_DIRECTORY}/freertos_cpp_wrappers/semaphore

    ${SOURCES_DIRECTORY}/tests

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