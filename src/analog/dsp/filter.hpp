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
#include <utility>
#include <vector>
#include <algorithm>
#include <functional>
#include <mutex>
#include "FreeRTOS.h"
#include "queue.h"
#include <arm_math.h>

#include "clamp_meter_concepts.hpp"
#include "semaphore.hpp"
#include "project_configs.hpp"
#include "task.hpp"
#include "queue.hpp"

class AbstractFilter {
  public:
    using ValueT               = float;   // todo: make class template
    using CoefficientT         = ValueT;
    using CoefficientsPack     = std::vector<CoefficientT>;
    using OutputReadyCallbackT = std::function<void()>;
    using BufferT              = std::vector<ValueT>;
    using InputBufferT         = std::shared_ptr<BufferT>;
    using OutputBufferT        = std::shared_ptr<BufferT>;
    using StateBuff            = std::vector<ValueT>;

    virtual ~AbstractFilter()        = 0;
    virtual void DoFilter() noexcept = 0;

    virtual void InputBufferIsReadyCallback() = 0;
    void SetOutputDataReadyCallback(OutputReadyCallbackT &&new_callback) noexcept { outputReadyCallback = std::move(new_callback); }
    virtual void PushData(ValueT new_data) = 0;

    virtual void SetOutputBuffer(OutputBufferT new_output_buffer) noexcept = 0;
    virtual void SetInputBuffer(InputBufferT new_input_buffer) noexcept    = 0;

    [[nodiscard]] OutputBufferT         GetOutputBuffer() const noexcept { return outputBuffer; }
    [[nodiscard]] InputBufferT          GetInputBuffer() const noexcept { return inputBuffer; }
    [[nodiscard]] virtual OutputBufferT CreateAndSetNewOutputBuffer() noexcept              = 0;
    [[nodiscard]] virtual InputBufferT  CreateAndSetNewInputBuffer() noexcept               = 0;
    [[nodiscard]] virtual bool          InputBufferShouldBeOfSpecificSize() const noexcept  = 0;
    [[nodiscard]] virtual bool          OutputBufferShouldBeOfSpecificSize() const noexcept = 0;
    [[nodiscard]] virtual size_t        GetDesiredInputBufferSize() const noexcept          = 0;
    [[nodiscard]] virtual size_t        GetDesiredOutputBufferSize() const noexcept         = 0;

  protected:
    CoefficientsPack              coefficients;
    InputBufferT                  inputBuffer;
    OutputBufferT                 outputBuffer;
    StateBuff                     stateBuffer{};
    OutputReadyCallbackT          outputReadyCallback;
    std::vector<ValueT>::iterator manualInsertionIterator{};
};

class FirDecimatingFilter : public AbstractFilter {
  public:
    using AbstractFilter::ValueT;
    using AbstractFilter::BufferT;
    using AbstractFilter::InputBufferT;
    using AbstractFilter::OutputBufferT;
    using AbstractFilter::CoefficientT;
    using AbstractFilter::CoefficientsPack;

    //    using StateBuffT          = std::array<ValueT, blockSize + numberOfCoefficients - 1>;

    explicit FirDecimatingFilter(CoefficientsPack &&new_coefficients, size_t decimating_factor, size_t block_size)
      : decimatingFactor{ decimating_factor }
      , blockSize{ block_size }
    {
        coefficients = std::move(new_coefficients);

        configASSERT(block_size % decimating_factor == 0);

        inputBuffer             = std::make_shared<BufferT>(block_size);
        manualInsertionIterator = inputBuffer->begin();
        outputBuffer            = std::make_shared<BufferT>(blockSize / decimatingFactor);
        stateBuffer.resize(block_size + coefficients.size() - 1);

        arm_fir_decimate_init_f32(&filter_instance,
                                  coefficients.size(),
                                  decimating_factor,
                                  coefficients.data(),
                                  stateBuffer.data(),
                                  block_size);
    }

    explicit FirDecimatingFilter(CoefficientsPack &&new_coefficients, size_t decimating_factor)
      : FirDecimatingFilter{ std::move(new_coefficients), decimating_factor, decimating_factor }
    { }

    explicit FirDecimatingFilter(const FirDecimatingFilter &other)
      : FirDecimatingFilter{ CoefficientsPack{ other.coefficients }, other.decimatingFactor, other.blockSize }
    { }
    FirDecimatingFilter &operator=(const FirDecimatingFilter &rhs)
    {
        *this = FirDecimatingFilter{ rhs };
        return *this;
    }
    FirDecimatingFilter(FirDecimatingFilter &&)            = default;
    FirDecimatingFilter &operator=(FirDecimatingFilter &&) = default;
    ~FirDecimatingFilter() override                        = default;

    void DoFilter() noexcept override
    {
        arm_fir_decimate_f32(&filter_instance, inputBuffer->data(), outputBuffer->data(), blockSize);
        AbstractFilter::outputReadyCallback();
    }
    void PushData(ValueT new_value) noexcept override
    {
        *manualInsertionIterator = new_value;
        manualInsertionIterator++;

        if (manualInsertionIterator == inputBuffer->end()) {
            DoFilter();
            manualInsertionIterator = inputBuffer->begin();
        }
    }
    void SetOutputBuffer(OutputBufferT new_output_buffer) noexcept override
    {
        configASSERT(new_output_buffer->size() >= (blockSize / decimatingFactor));
        outputBuffer = new_output_buffer;
    }
    void SetInputBuffer(InputBufferT new_input_buffer) noexcept override
    {
        configASSERT(new_input_buffer->size() % decimatingFactor == 0);
        inputBuffer = new_input_buffer;
        if (inputBuffer->size() != blockSize) {
            blockSize  = inputBuffer->size();
            auto dummy = CreateAndSetNewOutputBuffer();
        }

        manualInsertionIterator = inputBuffer->begin();
    }
    void InputBufferIsReadyCallback() noexcept override { DoFilter(); }

    [[nodiscard]] InputBufferT CreateAndSetNewInputBuffer() noexcept override
    {
        inputBuffer             = std::make_shared<BufferT>(blockSize);
        manualInsertionIterator = inputBuffer->begin();
        return inputBuffer;
    }
    [[nodiscard]] OutputBufferT CreateAndSetNewOutputBuffer() noexcept override
    {
        return outputBuffer = std::make_shared<BufferT>(blockSize / decimatingFactor);
    }

    [[nodiscard]] bool   InputBufferShouldBeOfSpecificSize() const noexcept override { return true; }
    [[nodiscard]] bool   OutputBufferShouldBeOfSpecificSize() const noexcept override { return true; }
    [[nodiscard]] size_t GetDesiredInputBufferSize() const noexcept override { return blockSize; }
    [[nodiscard]] size_t GetDesiredOutputBufferSize() const noexcept override { return blockSize / decimatingFactor; }

  private:
    arm_fir_decimate_instance_f32 filter_instance{};

    size_t decimatingFactor;
    size_t blockSize;
};

class BiquadCascadeDF2TFilter : public AbstractFilter {
  public:
    using AbstractFilter::ValueT;
    using AbstractFilter::BufferT;
    using AbstractFilter::InputBufferT;
    using AbstractFilter::OutputBufferT;
    using AbstractFilter::CoefficientT;
    using AbstractFilter::CoefficientsPack;

    explicit BiquadCascadeDF2TFilter(CoefficientsPack &&new_coefficients, size_t block_size)
      : stageNumber{ new_coefficients.size() / coefficients_per_stage }
      , blockSize{ block_size }
    {
        coefficients = std::move(new_coefficients);

        // assure number of coefficients supplied is correct
        configASSERT(coefficients.size() % coefficients_per_stage == 0);

        stateBuffer.resize(stageNumber * state_variables_per_stage);

        inputBuffer             = std::make_shared<BufferT>(blockSize);
        manualInsertionIterator = inputBuffer->begin();
        outputBuffer            = std::make_shared<BufferT>(blockSize);
        arm_biquad_cascade_df2T_init_f32(&filter_instance, stageNumber, coefficients.data(), stateBuffer.data());
    }

    explicit BiquadCascadeDF2TFilter(CoefficientsPack &&new_coefficients)
      : BiquadCascadeDF2TFilter{ std::move(new_coefficients), 1 }
    { }

    explicit BiquadCascadeDF2TFilter(const BiquadCascadeDF2TFilter &other)
      : BiquadCascadeDF2TFilter{ CoefficientsPack{ other.coefficients }, other.blockSize }
    { }
    BiquadCascadeDF2TFilter &operator=(const BiquadCascadeDF2TFilter &rhs)
    {
        *this = BiquadCascadeDF2TFilter{ rhs };
        return *this;
    }
    BiquadCascadeDF2TFilter(BiquadCascadeDF2TFilter &&rhs) = default;
    BiquadCascadeDF2TFilter &operator=(BiquadCascadeDF2TFilter &&rhs)
    {
        *this = BiquadCascadeDF2TFilter{ std::move(rhs) };
        return *this;
    }
    ~BiquadCascadeDF2TFilter() override = default;

    void DoFilter() noexcept override
    {
        arm_biquad_cascade_df2T_f32(&filter_instance, inputBuffer->data(), outputBuffer->data(), blockSize);
        AbstractFilter::outputReadyCallback();
    }
    void InputBufferIsReadyCallback() noexcept override { DoFilter(); }
    void PushData(ValueT new_value) noexcept override
    {
        *manualInsertionIterator = new_value;
        manualInsertionIterator++;

        if (manualInsertionIterator == inputBuffer->end()) {
            DoFilter();
            manualInsertionIterator = inputBuffer->begin();
        }
    }
    void SetOutputBuffer(OutputBufferT new_output_buffer) noexcept override
    {
        configASSERT(new_output_buffer->size() == blockSize);
        outputBuffer = new_output_buffer;
    }
    void SetInputBuffer(InputBufferT new_input_buffer) noexcept override
    {
        //        configASSERT(new_input_buffer->size() == blockSize);
        inputBuffer = new_input_buffer;
        blockSize   = inputBuffer->size();

        if (outputBuffer->size() != inputBuffer->size()) {
            auto dummy = CreateAndSetNewOutputBuffer();
        }

        manualInsertionIterator = inputBuffer->begin();
    }

    [[nodiscard]] InputBufferT CreateAndSetNewInputBuffer() noexcept override
    {
        inputBuffer             = std::make_shared<BufferT>(blockSize);
        manualInsertionIterator = inputBuffer->begin();
        return inputBuffer;
    }
    [[nodiscard]] OutputBufferT CreateAndSetNewOutputBuffer() noexcept override
    {
        return outputBuffer = std::make_shared<BufferT>(blockSize);
    }
    [[nodiscard]] bool   InputBufferShouldBeOfSpecificSize() const noexcept override { return false; }
    [[nodiscard]] bool   OutputBufferShouldBeOfSpecificSize() const noexcept override { return false; }
    [[nodiscard]] size_t GetDesiredInputBufferSize() const noexcept override { return blockSize; }
    [[nodiscard]] size_t GetDesiredOutputBufferSize() const noexcept override { return blockSize; }

  private:
    arm_biquad_cascade_df2T_instance_f32 filter_instance{};
    size_t                               stageNumber;
    size_t                               blockSize;

    auto constexpr static coefficients_per_stage    = 5;
    auto constexpr static state_variables_per_stage = 2;
    auto constexpr static minimal_block_size        = 1;
};

// todo:make double channel filter
template<typename ValueT>
class SuperFilterWithTask {
  public:
    using DataReadyCallback = AbstractFilter::OutputReadyCallbackT;
    using InputStream       = StreamBuffer<ValueT>;
    using OwnedQueue        = QueueHandle_t;
    using BorrowedQueue     = OwnedQueue;
    using Lock              = std::lock_guard<Mutex>;
    using BufferT           = AbstractFilter::BufferT;
    using InputBufferT      = AbstractFilter::InputBufferT;
    using OutputBufferT     = AbstractFilter::OutputBufferT;
    enum Configs {
        QueueMsgSize          = sizeof(ValueT),
        QueueSendTimeoutValue = 0,
        ReceiveTimeout        = portMAX_DELAY
    };

    SuperFilterWithTask() noexcept
      : inputStream{ std::make_shared<InputStream>(ProjectConfigs::ADCStreamBufferCapacity,
                                                   ProjectConfigs::ADCStreamBufferTriggeringSize) }
      , filterTask([this]() { this->InputTask(); },
                   ProjectConfigs::GetTaskStackSize(ProjectConfigs::Tasks::Filter),
                   ProjectConfigs::GetTaskPriority(ProjectConfigs::Tasks::Filter),
                   "filter input task")
    {
        filterTask.Suspend();
    }

    void InsertFilter(std::unique_ptr<AbstractFilter> &&filter) noexcept
    {
        filters.push_back(std::forward<decltype(filter)>(filter));

        if (filters.size() == 1)
            firstFilterInput = (*filters.begin())->GetInputBuffer();

        if (filters.size() >= 2) {
            auto one_before_back = filters.end() - 2;
            (*one_before_back)->SetOutputDataReadyCallback([this, one_ahead = filters.size() - 1]() {
                this->filters.at(one_ahead)->InputBufferIsReadyCallback();
            });

            ConnectTwoLastFilters();
        }
        filters.back()->SetOutputDataReadyCallback([this]() { LastFilterDataReadyCallback(); });
        lastFilterOutput = (*filters.back()).GetOutputBuffer();
    }
    void PushData(ValueT new_value) noexcept { (*filters.begin())->PushData(new_value); }
    void SetOutputQueue(BorrowedQueue new_output_queue) noexcept
    {
        Lock{ outputQueueMutex };
        outputQueue = new_output_queue;
    }
    void Resume() noexcept { filterTask.Resume(); }
    void Suspend() noexcept { filterTask.Suspend(); }

    [[nodiscard]] InputBufferT                 GetFirstBuffer() noexcept { return firstFilterInput; }
    [[nodiscard]] OutputBufferT                GetLastBuffer() noexcept { return lastFilterOutput; }
    [[nodiscard]] std::shared_ptr<InputStream> GetInputStreamBuffer() const noexcept { return inputStream; }
    [[nodiscard]] OwnedQueue                   GetOutputQueue() noexcept
    {
        Lock{ outputQueueMutex };
        return outputQueue;
    }

  protected:
    [[gnu::hot]] [[noreturn]] void InputTask() noexcept
    {
        while (true) {
            auto input = inputStream->template Receive<ProjectConfigs::ADCStreamBufferTriggeringSize>(ReceiveTimeout);

            std::transform(input.begin(), input.end(), firstFilterInput->begin(), [](auto &input) { return input; });

            (*filters.begin())->DoFilter();
        }
    }
    [[gnu::hot]] void LastFilterDataReadyCallback()
    {
        auto output_value = *(lastFilterOutput->begin());
        Lock{ outputQueueMutex };

        xQueueSend(outputQueue, static_cast<const void *>(&output_value), QueueSendTimeoutValue);
    }
    void ConnectTwoLastFilters() noexcept
    {
        configASSERT(filters.size() >= 2);

        auto one_before_back = filters.end() - 2;
        auto one_at_back     = filters.end() - 1;

        (*one_at_back)->SetInputBuffer((*one_before_back)->GetOutputBuffer());
    }

  private:
    std::shared_ptr<InputStream> inputStream;
    BorrowedQueue                outputQueue = nullptr;
    InputBufferT                 firstFilterInput;
    OutputBufferT                lastFilterOutput;

    std::vector<std::unique_ptr<AbstractFilter>> filters;

    Mutex outputQueueMutex;

    Task filterTask;
};

template<typename ValueT>
class SuperFilterNoTask {
  public:
    using DataReadyCallback = AbstractFilter::OutputReadyCallbackT;
    using BufferT           = AbstractFilter::BufferT;
    using InputBufferT      = AbstractFilter::InputBufferT;
    using OutputBufferT     = AbstractFilter::OutputBufferT;
    enum Configs {
        QueueMsgSize          = sizeof(ValueT),
        QueueSendTimeoutValue = 0,
        ReceiveTimeout        = portMAX_DELAY
    };

    void InsertFilter(std::unique_ptr<AbstractFilter> &&filter) noexcept
    {
        filters.push_back(std::forward<decltype(filter)>(filter));

        if (filters.size() == 1)
            firstFilterInput = (*filters.begin())->GetInputBuffer();

        if (filters.size() >= 2) {
            auto one_before_back = filters.end() - 2;
            (*one_before_back)->SetOutputDataReadyCallback([this, one_ahead = filters.size() - 1]() {
                this->filters.at(one_ahead)->InputBufferIsReadyCallback();
            });

            ConnectTwoLastFilters();
        }
        filters.back()->SetOutputDataReadyCallback([this]() { ; });
        lastFilterOutput = (*filters.back()).GetOutputBuffer();
    }
    void   PushData(ValueT new_value) noexcept { (*filters.begin())->PushData(new_value); }
    ValueT DoFilter() noexcept
    {
        (*filters.begin())->DoFilter();
        return *(lastFilterOutput->begin());
    }

    [[nodiscard]] InputBufferT  GetFirstBuffer() noexcept { return firstFilterInput; }
    [[nodiscard]] OutputBufferT GetLastBuffer() noexcept { return lastFilterOutput; }

  protected:
    void ConnectTwoLastFilters() noexcept
    {
        configASSERT(filters.size() >= 2);

        auto one_before_back = filters.end() - 2;
        auto one_at_back     = filters.end() - 1;

        (*one_at_back)->SetInputBuffer((*one_before_back)->GetOutputBuffer());
    }

  private:
    InputBufferT  firstFilterInput;
    OutputBufferT lastFilterOutput;

    std::vector<std::unique_ptr<AbstractFilter>> filters;
};
