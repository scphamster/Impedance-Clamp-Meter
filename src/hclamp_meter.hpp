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

#include <memory>
#include <mutex>
#include <cstdint>

#include "FreeRTOS.h"
#include "task.h"

#include "mutex/semaphore.hpp"
#include "clamp_meter_drawing.hpp"
#include "clamp_meter_concepts.hpp"

#include "menu_model.hpp"
#include "menu_model_item.hpp"
#include "menu_model_drawer.hpp"
// test
#include "timer.hpp"
#include "mcp23016_driver_fast.hpp"
#include "button.hpp"
#include "keyboard_fast.hpp"

[[noreturn]] void tasks_setup2();
extern "C" void   ClampMeterMeasurementsTaskWrapper(void *);
extern "C" void   ClampMeterDisplayMeasurementsTaskWrapper(void *);

template<typename Drawer, typename Reader>
class ClampMeterInTaskHandler;

template<typename Drawer, typename Sensor, KeyboardC Keyboard = Keyboard<MCP23016_driver, TimerFreeRTOS, MCP23016Button>>
class ClampMeter : private Sensor {
  public:
    ClampMeter(std::unique_ptr<Drawer> &&display_to_be_used, std::unique_ptr<Keyboard> &&new_keyboard)
      : mutex{ std::make_shared<Mutex>() }
      , timer{ pdMS_TO_TICKS(10) }
      , model{ std::make_shared<MenuModel<Keyboard>>(std::forward<decltype(new_keyboard)>(new_keyboard),
                                                                   mutex) }
      , drawer{ mutex, std::forward<decltype(display_to_be_used)>(display_to_be_used) }
    {
        auto top_item = std::make_shared<MenuModelPageItem>();
        top_item->SetPosition(MenuModelIndex{ { 0, 0 } });
        top_item->SetData(std::make_shared<MenuModelPageItemData>("dupa", 1));

        auto first_item = std::make_shared<MenuModelPageItem>();
        first_item->SetData(std::make_shared<MenuModelPageItemData>("dupa", 1));
        first_item->SetPosition(MenuModelIndex{ { 0, 0 } });

        top_item->InsertChild(first_item);
        auto children = top_item->GetChildren();
        auto name = children.at(0)->GetData()->GetName();

        model->SetTopLevelItem(top_item);

        drawer.SetModel(model);

    }

    ClampMeter(ClampMeter &&other)          = default;
    ClampMeter &operator=(ClampMeter &&rhs) = default;
    ~ClampMeter()                           = default;

    void StartMeasurementsTask() volatile
    {
        xTaskCreate(ClampMeterMeasurementsTaskWrapper,
                    "measure",
                    300,
                    std::remove_volatile_t<void *const>(this),
                    3,
                    nullptr);
    }

    void StartDisplayMeasurementsTask() volatile
    {
        xTaskCreate(ClampMeterDisplayMeasurementsTaskWrapper,
                    "display",
                    400,
                    std::remove_volatile_t<void *const>(this),
                    3,
                    nullptr);
    }

  protected:
    virtual void DisplayMeasurementsTask();
    virtual void MeasurementsTask() { vTaskDelay(pdMS_TO_TICKS(500)); }

  private:
    friend class ClampMeterInTaskHandler<Drawer, Sensor>;

    std::shared_ptr<Mutex>               mutex;
    TimerFreeRTOS                        timer;
    std::shared_ptr<MenuModel<Keyboard>> model;
    MenuModelDrawer<Drawer, Keyboard>    drawer;
};

template<typename Drawer, typename Sensor, KeyboardC Keyboard>
void
ClampMeter<Drawer, Sensor, Keyboard>::DisplayMeasurementsTask()
{
    drawer.DrawerTask();



    vTaskDelay(pdMS_TO_TICKS(500));
}

template<typename Drawer, typename Reader>
class ClampMeterInTaskHandler {
  public:
    //    ClampMeterInTaskHandler(const ClampMeter<Drawer, Reader> &instance)
    //      : ClampMeter<Drawer, Reader>{ instance }
    //    { }
    //
    //    void DisplayMeasurementsTask() override { ClampMeter<Drawer, Reader>::DisplayMeasurementsTask(); }
    //    void MeasurementsTask() override { ClampMeter<Drawer, Reader>::MeasurementsTask(); }

    ClampMeterInTaskHandler(ClampMeter<Drawer, Reader> &instance)
      : true_instance{ instance }
    { }

    void DisplayMeasurementsTask() { true_instance.DisplayMeasurementsTask(); }
    void MeasurementsTask() { true_instance.MeasurementsTask(); }

  protected:
  private:
    ClampMeter<Drawer, Reader> &true_instance;
};
