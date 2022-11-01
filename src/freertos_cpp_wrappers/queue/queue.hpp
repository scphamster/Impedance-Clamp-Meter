#pragma once
#include "compiler_compatibility_workaround.hpp"

#include <deque>
#include <mutex>

#include "FreeRTOS.h"
#include "FreeRTOS/include/queue.h"
#include "semaphore.hpp"
#include "FreeRTOS/include/stream_buffer.h"

template<typename ItemType>
class Queue {
  public:
    using TimeT = portTickType;

    Queue(size_t queue_length)
      : handle{ xQueueCreate(queue_length, sizeof(ItemType)) }
    {
        configASSERT(handle != nullptr);
    }
    ~Queue() noexcept
    {
        configASSERT(handle != nullptr);
        vQueueDelete(handle);
    }

    ItemType Receive(TimeT timeout = portMAX_DELAY)
    {
        ItemType new_item;
        configASSERT(xQueueReceive(handle, &new_item, timeout) == pdTRUE); //todo: handle exception
        return new_item;
    }

    bool Send(ItemType const &item, TimeT timeout) const noexcept
    {
        return (xQueueSend(handle, static_cast<const void *>(&item), timeout) == pdTRUE) ? true : false;
    }
    bool SendImmediate(ItemType const &item) const noexcept { return Send(item, 0); }

  private:
    QueueHandle_t handle = nullptr;
};

template<typename ItemType>
class Deque {
  public:
  protected:
  private:
    Mutex                pushMutex;
    Mutex                popMutex;
    std::deque<ItemType> queue;
};

template<typename ItemType>
class StreamBuffer {
  public:
    using TimeoutMsec = portTickType;

    StreamBuffer(size_t buffer_items_capacity, size_t receiver_unblock_triggering_size = 1) noexcept
      : handle{ xStreamBufferCreate(buffer_items_capacity * sizeof(ItemType), receiver_unblock_triggering_size * sizeof(ItemType)) }
    {
        configASSERT(handle != nullptr);
    }

    template<size_t NumberOfItems>
    void Send(std::array<ItemType, NumberOfItems> &buffer, TimeoutMsec timeout) noexcept
    {
        xStreamBufferSend(handle, buffer.data(), sizeof(buffer), timeout);
    }
    void Send(ItemType &&buffer, TimeoutMsec timeout) noexcept { xStreamBufferSend(handle, &buffer, sizeof(buffer), timeout); }
    bool Reset() const noexcept { return (xStreamBufferReset(handle) == pdPASS) ? true : false; }
    template<typename T>
    BaseType_t SendFromISR(T &&buffer) noexcept
    {
        BaseType_t higher_prio_task_voken;
        xStreamBufferSendFromISR(handle, &buffer, sizeof(buffer), &higher_prio_task_voken);
        return higher_prio_task_voken;
    }

    ItemType Receive(TimeoutMsec timeout) noexcept
    {
        ItemType buffer;
        xStreamBufferReceive(handle, &buffer, sizeof(buffer), timeout);
        return buffer;
    }
    template<size_t NumberOfItems>
    std::array<ItemType, NumberOfItems> Receive(TimeoutMsec timeout) noexcept
    {
        std::array<ItemType, NumberOfItems> buffer;
        auto received_number_of_bytes = xStreamBufferReceive(handle, buffer.data(), buffer.size() * sizeof(ItemType), timeout);

        //        configASSERT(buffer.size() * sizeof(ItemType) == received_number_of_bytes);
        return buffer;
    }
    template<size_t NumberOfItems>
    std::array<ItemType, NumberOfItems> ReceiveBlocking(TimeoutMsec timeout) noexcept
    {
        std::array<ItemType, NumberOfItems> buffer;
        auto constexpr buffer_size_in_bytes = buffer.size() * sizeof(ItemType);

        for (size_t received_number_of_bytes = 0; received_number_of_bytes != buffer_size_in_bytes;) {
            received_number_of_bytes = xStreamBufferReceive(handle, buffer.data(), buffer.size() * sizeof(ItemType), timeout);
        }

        return buffer;
    }

  private:
    StreamBufferHandle_t handle;
};