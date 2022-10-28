#pragma once
#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#ifdef printf
#undef printf
#endif

#include <array>
#include <functional>
#include <memory>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "amplifier_controller.hpp"

class SensorData {
  public:
    using ValueT = float;
    using QueueT = QueueHandle_t;

    void SetNewValue(ValueT value) { }

    void SetRawValue_I(ValueT new_value_i) noexcept { rawValue_I = new_value_i; }
    void SetRawValue_Q(ValueT new_value_q) noexcept { rawValue_Q = new_value_q; }
    void SetRawAbsoluteValue(ValueT new_absolutevalue) noexcept { rawAbsoluteValue = new_absolutevalue; }
    void SetRawDegree(ValueT new_degree) noexcept { rawDegree = new_degree; }
    void SetTrueValue_I(ValueT new_true_value_i) noexcept { trueValue_I = new_true_value_i; }
    void SetTrueValue_Q(ValueT new_true_value_q) noexcept { trueValue_Q = new_true_value_q; }
    void SetTrueAbsoluteValue(ValueT new_true_absolutevalue) noexcept { trueAbsoluteValue = new_true_absolutevalue; }
    void SetTrueDegree(ValueT new_true_degree) noexcept { trueDegree = new_true_degree; }

  private:
    friend class SensorController;

    QueueT inputQueue;
    QueueT outputQueue;

    ValueT rawValue_I{};
    ValueT rawValue_Q{};
    ValueT rawAbsoluteValue{};
    ValueT rawDegree{};

    ValueT trueValue_I{};
    ValueT trueValue_Q{};
    ValueT trueAbsoluteValue{};
    ValueT trueDegree{};
};

class SensorController {
  public:
    using ValueT = float;
    using QueueT = QueueHandle_t;

    [[noreturn]] void MainTask()
    {
        ValueT newValue;

        while (true) {
            xQueueReceive(data.inputQueue, &newValue, portMAX_DELAY);
            amplifierController.ForwardAmplitudeValueToAGCIfEnabled(newValue);
        }
    }

    void SetInputQueue(QueueT new_queue) noexcept { data.inputQueue = new_queue; }
    void SetOutputQueue(QueueT new_queue) noexcept { data.outputQueue = new_queue; }

    [[nodiscard]] QueueT GetInputQueue() const noexcept { return data.inputQueue; }
    [[nodiscard]] QueueT GetOutputQueue() const noexcept { return data.outputQueue; }

  private:
    SensorData          data;
    AmplifierController amplifierController;
};