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
    using Item  = MenuModelPageItem<Keyboard>;
    using Model = MenuModel<Keyboard>;

    ClampMeter(std::unique_ptr<Drawer> &&display_to_be_used, std::unique_ptr<Keyboard> &&new_keyboard)
      : mutex{ std::make_shared<Mutex>() }
      , timer{ pdMS_TO_TICKS(10) }
      , model{ std::make_shared<Model>(mutex) }
      , drawer{ mutex,
                std::forward<decltype(display_to_be_used)>(display_to_be_used),
                std::forward<decltype(new_keyboard)>(new_keyboard) }
      , shared_data{ std::make_shared<UniversalType>(static_cast<int>(5)) }
    {
        auto dummy_item01 = std::make_shared<Item>(model);
        dummy_item01->SetName("sub page");
        dummy_item01->SetData(MenuModelPageItemData{ std::make_shared<UniversalType>(10) });
        dummy_item01->SetIndex(0);

        auto dummy_item0 = std::make_shared<Item>(model);
        dummy_item0->SetData(MenuModelPageItemData{ shared_data });
        dummy_item0->SetName("dummy item");
        dummy_item0->SetIndex(0);
        dummy_item0->InsertChild(dummy_item01);

        auto top_item = std::make_shared<Item>(model);
        top_item->SetIndex(0);
        top_item->SetData(MenuModelPageItemData{ std::make_shared<UniversalType>(1) });
        top_item->SetName("Main Page");
        top_item->InsertChild(dummy_item0);

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

    std::shared_ptr<UniversalType> shared_data;

    int counter = 0;
};

template<typename Drawer, typename Sensor, KeyboardC Keyboard>
void
ClampMeter<Drawer, Sensor, Keyboard>::DisplayMeasurementsTask()
{
    drawer.DrawerTask();

    *shared_data = counter;

    counter++;
    if (counter == 100)
        counter = 0;
    vTaskDelay(pdMS_TO_TICKS(100));
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
