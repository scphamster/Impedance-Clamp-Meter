#pragma once
#include <type_traits>

namespace ProjectConfigs
{
enum class TaskStackSize {
    Main              = 400,
    Display           = 300,
    ClampDriverSensor = 300,
    SensorInput       = 1000,
    SensorFromFilter  = 300,
    Filter            = 600,
    MCP23016          = 300
};
enum class TaskPriority {
    Main              = 3,
    Display           = 1,
    ClampDriverSensor = 3,
    SensorInput       = 4,
    SensorFromFilter  = 4,
    Filter            = 3,
    MCP23016          = 2
};

enum class QueueSize {
    FromAdc = 200
};

enum class InterruptPriority {
    ADC      = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY,
    Keyboard = 13
};

enum class Interrupts {
    ADC,
    Keyboard
};

enum {
    SuperFilterFirstBufferSize = 100,
    ADCAddress         = 0x40,
    ADCStreamBufferCapacity = 200,
    ADCStreamBufferTriggeringSize = SuperFilterFirstBufferSize,

    DisplayDrawingDrawingPeriodMs = pdMS_TO_TICKS(200),
};

enum class Tasks {
    Main,
    Display,
    ClampDriverSensor,
    SensorInput,
    SensorFromFilter,
    Filter,
    MCP23016
};

constexpr std::underlying_type_t<TaskStackSize>
GetTaskStackSize(Tasks task)
{
    using EnumClassOfInterest = TaskStackSize;
    using UnderType           = std::underlying_type_t<EnumClassOfInterest>;

    switch (task) {
    case Tasks::Main: return static_cast<UnderType>(EnumClassOfInterest::Main);
    case Tasks::Display: return static_cast<UnderType>(EnumClassOfInterest::Display);
    case Tasks::ClampDriverSensor: return static_cast<UnderType>(EnumClassOfInterest::ClampDriverSensor);
    case Tasks::SensorInput: return static_cast<UnderType>(EnumClassOfInterest::SensorInput);
    case Tasks::SensorFromFilter: return static_cast<UnderType>(EnumClassOfInterest::SensorFromFilter);
    case Tasks::Filter: return static_cast<UnderType>(EnumClassOfInterest::Filter);
    case Tasks::MCP23016: return static_cast<UnderType>(EnumClassOfInterest::MCP23016);
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
    case Tasks::SensorInput: return static_cast<UnderType>(EnumClassOfInterest::SensorInput);
    case Tasks::SensorFromFilter: return static_cast<UnderType>(EnumClassOfInterest::SensorFromFilter);
    case Tasks::Filter: return static_cast<UnderType>(EnumClassOfInterest::Filter);
    case Tasks::MCP23016: return static_cast<UnderType>(EnumClassOfInterest::MCP23016);
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