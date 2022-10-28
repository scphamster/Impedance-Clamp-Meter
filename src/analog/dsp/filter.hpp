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

#include <cstdio>
#include <memory>
#include <arm_math.h>
#include <vector>
#include <functional>
#include "FreeRTOS.h"
#include "queue.h"

#include "clamp_meter_concepts.hpp"

class AbstractFilter {
  public:
    using OutputReadyCallbackT = std::function<void()>;

    virtual void Initialize() noexcept = 0;
    virtual void DoFilter() noexcept   = 0;
    virtual ~AbstractFilter()          = 0;

    virtual void InputBufferIsReadyCallback() = 0;
    virtual void SetOutputDataReadyCallback(OutputReadyCallbackT &&new_callback) noexcept
    {
        outputReadyCallback = std::move(new_callback);
    }

  protected:
    OutputReadyCallbackT outputReadyCallback;
};

template<size_t blockSize, size_t numberOfCoefficients, size_t decimatingFactor>
class FirDecimatingFilter : public AbstractFilter {
  public:
    using ValueT              = float;
    using CoefficientT        = float;
    using CoefficientsT       = std::array<CoefficientT, numberOfCoefficients>;
    using StateBuffT          = std::array<ValueT, blockSize + numberOfCoefficients - 1>;
    using InputBufferT        = std::shared_ptr<std::array<ValueT, blockSize>>;
    using OutputBufferSubType = std::array<ValueT, blockSize / decimatingFactor>;
    using OutputBufferT       = std::shared_ptr<std::array<ValueT, blockSize / decimatingFactor>>;

    FirDecimatingFilter() = delete;
    FirDecimatingFilter(CoefficientsT &&new_coefficients, InputBufferT new_input_buffer)
      : coefficients{ std::move(new_coefficients) }
      , inputBuffer{ new_input_buffer }
      , outputBuffer{ std::make_shared<OutputBufferSubType>() }
    {
        Initialize();
    }
    FirDecimatingFilter(const FirDecimatingFilter &)            = delete;
    FirDecimatingFilter &operator=(const FirDecimatingFilter &) = delete;
    FirDecimatingFilter(FirDecimatingFilter &&)                 = default;
    FirDecimatingFilter &operator=(FirDecimatingFilter &&)      = default;
    ~FirDecimatingFilter() override                             = default;

    void Initialize() noexcept override
    {
        arm_fir_decimate_init_f32(&filter_instance,
                                  numberOfCoefficients,
                                  decimatingFactor,
                                  coefficients.data(),
                                  stateBuffer.data(),
                                  blockSize);
    }

    void DoFilter() noexcept override
    {
        arm_fir_decimate_f32(&filter_instance, inputBuffer->data(), outputBuffer->data(), blockSize);
        AbstractFilter::outputReadyCallback();
    }
    void InputBufferIsReadyCallback() noexcept override { DoFilter(); }

    [[nodiscard]] OutputBufferT GetOutputBuffer() noexcept { return outputBuffer; }

  private:
    arm_fir_decimate_instance_f32 filter_instance{};
    CoefficientsT                 coefficients;
    StateBuffT                    stateBuffer{};
    InputBufferT                  inputBuffer;
    OutputBufferT                 outputBuffer;
};

template<size_t numberOfStages, size_t filterBlockSize>
class BiquadCascadeDF2TFilter : public AbstractFilter {
  public:
    using ValueT              = float;
    using CoefficientT        = float;
    using CoefficientsT       = std::array<CoefficientT, numberOfStages * 5>;
    using StateBuffT          = std::array<ValueT, numberOfStages * 2>;
    using InputBufferT        = std::shared_ptr<std::array<ValueT, filterBlockSize>>;
    using OutputBufferSubType = std::array<ValueT, filterBlockSize>;
    using OutputBufferT       = std::shared_ptr<OutputBufferSubType>;

    BiquadCascadeDF2TFilter(CoefficientsT &&new_coefficients, InputBufferT new_input_buffer)
      : coefficients{ std::move(new_coefficients) }
      , inputBuffer{ new_input_buffer }
      , outputBuffer{ std::make_shared<OutputBufferSubType>() }
    {
        Initialize();
    }

    BiquadCascadeDF2TFilter(const BiquadCascadeDF2TFilter &)            = delete;
    BiquadCascadeDF2TFilter &operator=(const BiquadCascadeDF2TFilter &) = delete;
    BiquadCascadeDF2TFilter(BiquadCascadeDF2TFilter &&)                 = default;
    BiquadCascadeDF2TFilter &operator=(BiquadCascadeDF2TFilter &&)      = default;
    ~BiquadCascadeDF2TFilter() override                                 = default;

    void Initialize() noexcept override
    {
        arm_biquad_cascade_df2T_init_f32(&filter_instance, numberOfStages, coefficients.data(), stateBuffer.data());
    }
    void DoFilter() noexcept override
    {
        arm_biquad_cascade_df2T_f32(&filter_instance, inputBuffer->data(), outputBuffer->data(), filterBlockSize);
        AbstractFilter::outputReadyCallback();
    }

    void InputBufferIsReadyCallback() noexcept override { DoFilter(); }

    [[nodiscard]] OutputBufferT GetOutputBuffer() noexcept { return outputBuffer; }

  private:
    arm_biquad_cascade_df2T_instance_f32 filter_instance{};
    CoefficientsT                        coefficients;
    StateBuffT                           stateBuffer{};
    InputBufferT                         inputBuffer;
    OutputBufferT                        outputBuffer;
};

extern "C" void SuperFilterTask(void *param);

template<typename ValueT, size_t firstBufferSize, size_t lastBufferSize>
class SuperFilter {
  public:
    using FirstBufferT      = std::shared_ptr<std::array<ValueT, firstBufferSize>>;
    using LastBufferT       = std::shared_ptr<std::array<ValueT, lastBufferSize>>;
    using DataReadyCallback = std::function<void()>;
    using QueueT            = QueueHandle_t;

    enum Configs {
        QueueMsgSize          = sizeof(ValueT),
        QueueSendTimeoutValue = 0,
        StackDepth            = 400,
        TaskPriority          = 4
    };

    SuperFilter(size_t input_queue_size, size_t output_queue_size)
      : inputQueue{ xQueueCreate(input_queue_size, QueueMsgSize) }
      , outputQueue{ xQueueCreate(output_queue_size, QueueMsgSize) }
      , firstBuffer{ std::make_shared<std::array<ValueT, firstBufferSize>>() }
      , lastBuffer{ std::make_shared<std::array<ValueT, lastBufferSize>>() }
    {
        xTaskCreate(SuperFilterTask, "superFilter", StackDepth, this, TaskPriority, nullptr);
    }

    [[noreturn]] void InputQueueTask()
    {
        while (true) {
            auto new_input_value = ValueT{};

            xQueueReceive(inputQueue, &new_input_value, portMAX_DELAY);
            Push(new_input_value);
        }
    }

    void Push(ValueT new_value) noexcept
    {
        (*firstBuffer)[bufferIteratorPosition++] = new_value;
        if (bufferIteratorPosition == firstBufferSize) {
            filters.at(0)->DoFilter();
            bufferIteratorPosition = 0;
        }
    }
    template<template<size_t...> class FilterType, size_t... sizes, typename... Args>
    void EmplaceFilter(Args &&...args)
    {
        if (filters.empty()) {
            filters.push_back(
              std::unique_ptr<AbstractFilter>(new FilterType<sizes...>(std::forward<Args>(args)..., firstBuffer)));
        }
        else {
            //            filters.push_back(std::unique_ptr<AbstractFilter>(
            //              new FilterType<sizes...>(std::forward<Args>(args)..., )));
        }
    }
    void InsertFilter(std::unique_ptr<AbstractFilter> &&filter) noexcept
    {
        filters.push_back(std::forward<decltype(filter)>(filter));

        if (filters.size() >= 2) {
            auto one_before_back = filters.end() - 2;
            (*one_before_back)->SetOutputDataReadyCallback([this, one_ahead = filters.size() - 1]() {
                this->filters.at(one_ahead)->InputBufferIsReadyCallback();
            });
        }

        filters.back()->SetOutputDataReadyCallback([this]() { LastFilterDataReadyCallback(); });
    }

    [[nodiscard]] FirstBufferT GetFirstBuffer() noexcept { return firstBuffer; }
    [[nodiscard]] LastBufferT  GetLastBuffer() noexcept { return lastBuffer; }
    [[nodiscard]] QueueT       GetInputQueue() const noexcept { return inputQueue; }
    [[nodiscard]] QueueT       GetOutputQueue() const noexcept { return outputQueue; }

  protected:
    void LastFilterDataReadyCallback()
    {
        auto new_output_value = (*lastBuffer)[0];
        xQueueSend(outputQueue, &new_output_value, QueueSendTimeoutValue);
    }

  private:
    QueueT inputQueue  = nullptr;
    QueueT outputQueue = nullptr;

    FirstBufferT firstBuffer;
    LastBufferT  lastBuffer;

    std::vector<std::unique_ptr<AbstractFilter>> filters;

    size_t bufferIteratorPosition = 0;
};