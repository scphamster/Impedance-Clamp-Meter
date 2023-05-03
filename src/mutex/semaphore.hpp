#pragma once
#include <memory>
#include "FreeRTOS.h"
#include <bits/allocator.h>
class Mutex {
  public:
    Mutex();
    Mutex(const Mutex &);
    Mutex &operator=(const Mutex &);
    Mutex(Mutex &&);
    Mutex &operator=(Mutex &&);
    ~Mutex();

    bool lock(portTickType ticks_to_wait = portMAX_DELAY) noexcept;
    bool unlock() noexcept;

  private:
    class MutexImpl;
    std::unique_ptr<MutexImpl> impl;
};
