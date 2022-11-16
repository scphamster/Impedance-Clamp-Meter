#pragma once

#include "compiler_compatibility_workaround.hpp"
#include "project_configs.hpp"

#include "FreeRTOS.h"

#include "task.hpp"
#include "pio.h"
#include "pmc.h"

class BuzzerTask {
  public:
    BuzzerTask(int pin_number, float frequency)
      : task{ [this]() { _Task(); },
              ProjectConfigs::GetTaskStackSize(ProjectConfigs::Tasks::Buzzer),
              ProjectConfigs::GetTaskPriority(ProjectConfigs::Tasks::Buzzer),
              "buzzer" }
      , pinNumber{ pin_number }
      , periodMs{ 1000.f / frequency }
    {
        pio_set_output(PIOD, (1 << 12), 0, 0, 0);
        //        pio_configure_pin(pin_number, PIO_TYPE_PIO_OUTPUT_0 bitor PIO_DEFAULT);
    }

    void Start() noexcept { task.Resume(); }
    void Stop() noexcept
    {
        task.Suspend();
        pio_clear(PIOD, 1 << 12);
    }

  protected:
    [[noreturn]] void _Task() noexcept
    {
        while (true) {
            if (pinIsHigh) {
                pio_clear(PIOD, 1 << 12);
                pinIsHigh = false;
            }
            else {
                pio_set(PIOD, 1 << 12);
                pinIsHigh = true;
            }

            Task::DelayMsUntil(periodMs);
        }
    }

  private:
    Task  task;
    int   pinNumber;
    float periodMs;

    bool pinIsHigh = false;
};