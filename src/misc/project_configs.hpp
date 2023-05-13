#pragma once
#include "../../../../../tools/arm_gcc_toolchain/10 2021.10/arm-none-eabi/include/c++/10.3.1/type_traits"
#include "../asf_lib/src/ASF/thirdparty/FreeRTOS/include/FreeRTOS.h"
#include "../asf_lib/src/ASF/thirdparty/FreeRTOS/FreeRTOSConfig.h"

namespace ProjectConfigs
{
#ifdef DEBUG
enum class TaskStackSize {
    Display                = 500,
    ClampDriverSensor      = 400,
    ClampDriverCalibration = 500,
    SensorInput            = 400,
    SensorFromFilter       = configMINIMAL_STACK_SIZE,
    Filter                 = configMINIMAL_STACK_SIZE,
    MCP23016               = configMINIMAL_STACK_SIZE,
    Buzzer                 = configMINIMAL_STACK_SIZE,
};
#else
enum class TaskStackSize {
    Display                = 350,
    ClampDriverSensor      = 550,
    ClampDriverCalibration = 500,
    SensorInput            = 400,
    SensorFromFilter       = configMINIMAL_STACK_SIZE,
    Filter                 = configMINIMAL_STACK_SIZE,
    MCP23016               = configMINIMAL_STACK_SIZE,
    Buzzer                 = configMINIMAL_STACK_SIZE,
};
#endif
enum class TaskPriority {
    Main                   = 3,
    Display                = 1,
    ClampDriverSensor      = 3,
    ClampDriverCalibration = 3,
    SensorInput            = 4,
    SensorFromFilter       = 4,
    Filter                 = 3,
    MCP23016               = 2,
    Buzzer                 = 2,
};

enum class QueueSize {
    FromAdc = 200
};

enum class InterruptPriority {
    ADC      = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY,
    Keyboard = 13,
    DaccPdc  = 2,
};

enum class Interrupts {
    ADC,
    Keyboard,
    DaccPdc
};

enum {
    SuperFilterFirstBufferSize    = 100,
    ADCAddress                    = 0x40,
    ADCStreamBufferCapacity       = 200,
    ADCStreamBufferTriggeringSize = SuperFilterFirstBufferSize,

    DisplayDrawingDrawingPeriodMs = 200,
    SensorFirstFilterBufferSize   = 100,
    FromSensorOutputQueueLength   = 10,
    GeneratorAmplitude            = 50,
};

enum class Tasks {
    Main,
    Display,
    ClampDriverSensor,
    ClampDriverCalibration,
    SensorInput,
    SensorFromFilter,
    Filter,
    MCP23016,
    Buzzer
};

template<typename EnumClassType>
consteval std::underlying_type_t<EnumClassType>
ToUnderlying(EnumClassType enum_value)
{
    return static_cast<std::underlying_type_t<EnumClassType>>(enum_value);
}

constexpr std::underlying_type_t<TaskStackSize>
GetTaskStackSize(Tasks task)
{
    using EnumClassOfInterest = TaskStackSize;
    using UnderType           = std::underlying_type_t<EnumClassOfInterest>;

    switch (task) {
    case Tasks::Display: return static_cast<UnderType>(EnumClassOfInterest::Display);
    case Tasks::ClampDriverSensor: return static_cast<UnderType>(EnumClassOfInterest::ClampDriverSensor);
    case Tasks::ClampDriverCalibration: return static_cast<UnderType>(EnumClassOfInterest::ClampDriverCalibration);
    case Tasks::SensorInput: return static_cast<UnderType>(EnumClassOfInterest::SensorInput);
    case Tasks::SensorFromFilter: return static_cast<UnderType>(EnumClassOfInterest::SensorFromFilter);
    case Tasks::Filter: return static_cast<UnderType>(EnumClassOfInterest::Filter);
    case Tasks::MCP23016: return static_cast<UnderType>(EnumClassOfInterest::MCP23016);
    case Tasks::Buzzer: return static_cast<UnderType>(EnumClassOfInterest::Buzzer);
    default: return -1;
    }
}

constexpr std::underlying_type_t<TaskPriority>
GetTaskPriority(Tasks task)
{
    using EnumClassOfInterest = TaskPriority;
    using UnderType           = std::underlying_type_t<EnumClassOfInterest>;

    switch (task) {
    case Tasks::Main: return static_cast<UnderType>(EnumClassOfInterest::Main);
    case Tasks::Display: return static_cast<UnderType>(EnumClassOfInterest::Display);
    case Tasks::ClampDriverSensor: return static_cast<UnderType>(EnumClassOfInterest::ClampDriverSensor);
    case Tasks::ClampDriverCalibration: return static_cast<UnderType>(EnumClassOfInterest::ClampDriverCalibration);
    case Tasks::SensorInput: return static_cast<UnderType>(EnumClassOfInterest::SensorInput);
    case Tasks::SensorFromFilter: return static_cast<UnderType>(EnumClassOfInterest::SensorFromFilter);
    case Tasks::Filter: return static_cast<UnderType>(EnumClassOfInterest::Filter);
    case Tasks::MCP23016: return static_cast<UnderType>(EnumClassOfInterest::MCP23016);
    case Tasks::Buzzer: return static_cast<UnderType>(EnumClassOfInterest::Buzzer);
    default: return -1;
    }
}

constexpr std::underlying_type_t<InterruptPriority>
GetInterruptPriority(Interrupts interrupt)
{
    using EnumClassOfInterest = InterruptPriority;
    using UnderType           = std::underlying_type_t<EnumClassOfInterest>;

    switch (interrupt) {
    case Interrupts::ADC: return static_cast<UnderType>(InterruptPriority::ADC);
    case Interrupts::Keyboard: return static_cast<UnderType>(InterruptPriority::Keyboard);
    case Interrupts::DaccPdc: return ToUnderlying(InterruptPriority::DaccPdc);
    default: return -1;
    }
}

constexpr std::underlying_type_t<QueueSize>
GetQueueSize(QueueSize queue)
{
    using EnumClassOfInterest = decltype(queue);
    using UnderType           = std::underlying_type_t<EnumClassOfInterest>;

    switch (queue) {
    case QueueSize::FromAdc: return static_cast<UnderType>(QueueSize::FromAdc);
    default: return -1;
    }
}
}