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
#include "queue.h"

#include "mutex/semaphore.hpp"
#include "clamp_meter_drawing.hpp"
#include "clamp_meter_concepts.hpp"

#include "menu_model.hpp"
#include "menu_model_item.hpp"
#include "menu_model_drawer.hpp"

#include "clamp_meter_driver.hpp"
#include "task.hpp"
#include "semaphore/semaphore.hpp"

// todo: cleanup
#include "timer.hpp"
#include "mcp23016_driver_fast.hpp"
#include "keyboard_fast.hpp"

// todo refactor headers below into statics
#include "external_periph_ctrl.h"
#include "signal_conditioning.h"

[[noreturn]] void tasks_setup2();

template<typename DrawerT, KeyboardC Keyboard = Keyboard<MCP23016_driver, TimerFreeRTOS, MCP23016Button>>
class TasksControllerImplementation : public std::enable_shared_from_this<TasksControllerImplementation<DrawerT, Keyboard>> {
  public:
    using Page      = MenuModelPage<Keyboard>;
    using Menu      = MenuModel<Keyboard>;
    using PageDataT = std::shared_ptr<UniversalSafeType>;
    using Drawer    = std::unique_ptr<DrawerT>;

    // fixme: make corrections of init order
    TasksControllerImplementation(std::unique_ptr<DrawerT> &&display_to_be_used, std::unique_ptr<Keyboard> &&new_keyboard)
      : drawer{ std::forward<decltype(display_to_be_used)>(display_to_be_used), std::forward<decltype(new_keyboard)>(new_keyboard) }
      , drawerTask{ [this]() { this->DisplayTask(); },
                    ProjectConfigs::GetTaskStackSize(ProjectConfigs::Tasks::Display),
                    ProjectConfigs::GetTaskPriority(ProjectConfigs::Tasks::Display),
                    "display" }
      , clampMeter{ vOut, vShunt, zClamp, drawer.CreateAndGetDialog() }
      , vOut{ std::make_shared<UniversalSafeType>(static_cast<float>(0)) }        // test
      , vShunt{ std::make_shared<UniversalSafeType>(static_cast<float>(0)) }      // test
      , zClamp{ std::make_shared<UniversalSafeType>(static_cast<float>(0)) }      // test
      , sensorMag{ std::make_shared<UniversalSafeType>(static_cast<float>(0)) }   // test
      , sensorPhi{ std::make_shared<UniversalSafeType>(static_cast<float>(0)) }   // test
      , menu{ std::make_shared<Menu>() }
    {
        InitializeMenu();
    }
    TasksControllerImplementation(TasksControllerImplementation &&other)          = default;
    TasksControllerImplementation &operator=(TasksControllerImplementation &&rhs) = default;
    ~TasksControllerImplementation()                                              = default;

    void ShowMessage(std::string                    message,
                     std::shared_ptr<UniversalType> value,
                     std::shared_ptr<bool>          decision,
                     std::shared_ptr<Semaphore>     decision_made) noexcept
    {
        auto new_dialog = std::make_shared<MenuModelDialog>(message, value, decision, decision_made);
        drawer.ShowDialog(std::move(new_dialog));
    }

  protected:
    // tasks
    [[noreturn]] void DisplayTask() noexcept
    {
        size_t counter = 0;

        //        auto dialog = drawer.CreateAndGetDialog();
        //        dialog->SetMsg("dupa 12345567");
        //        auto val = std::make_shared<UniversalSafeType>(36.5f);
        //        dialog->SetValue(val);

        while (true) {
            drawer.DrawerTask();
            Task::DelayMsUntil(ProjectConfigs::DisplayDrawingDrawingPeriodMs);

            //            counter++;
            //            if (counter == 5) {
            //                dialog->Show();
            //            }
        }
    }

    // initializers
    void InitializeMenu() noexcept
    {
        // measurements menu
        auto vout_info = std::make_shared<Page>(menu);
        vout_info->SetName("V out");
        vout_info->SetData(vOut);
        vout_info->SetIndex(0);

        auto z_overall = std::make_shared<Page>(menu);
        z_overall->SetName("Z Overall");
        z_overall->SetData(vShunt);
        z_overall->SetIndex(1);

        auto z_clamp = std::make_shared<Page>(menu);
        z_clamp->SetName("Z Clamp");
        z_clamp->SetData(zClamp);
        z_clamp->SetIndex(2);

        auto measurements_page = std::make_shared<Page>(menu);
        measurements_page->SetIndex(0);
        measurements_page->SetName("Measurement");
        measurements_page->InsertChild(vout_info);
        measurements_page->InsertChild(z_overall);
        measurements_page->InsertChild(z_clamp);
        measurements_page->SetKeyCallback(Keyboard::ButtonName::F1, [this]() { clampMeter.StartNormalModeOperation(); });
        measurements_page->SetKeyCallback(Keyboard::ButtonName::F2, [this]() { clampMeter.StopMeasurements(); });
        measurements_page->SetKeyCallback(Keyboard::ButtonName::F3, [this]() { clampMeter.SwitchToNextSensor(); });

        // calibration menu
        auto start_calibration = std::make_shared<Page>(menu);
        start_calibration->SetName("Press F1 to start calibration");
        start_calibration->SetHeader("Calibration");

        auto calibration_page = std::make_shared<Page>(menu);
        calibration_page->SetIndex(1);
        calibration_page->SetName("Calibration");
        calibration_page->InsertChild(start_calibration);
        calibration_page->SetKeyCallback(Keyboard::ButtonName::F1, [this]() { clampMeter.StartCalibration(); });

        auto main_page = std::make_shared<Page>(menu);
        main_page->SetName("Main");
        main_page->SetHeader(" ");
        main_page->InsertChild(measurements_page);
        main_page->InsertChild(calibration_page);

        menu->SetTopLevelItem(main_page);
        drawer.SetModel(menu);
    }

  private:
    MenuModelDrawer<DrawerT, Keyboard> drawer;

    // todo: to be deleted from here
    PageDataT vOut;
    PageDataT vShunt;

    PageDataT zClamp;
    PageDataT sensorMag;

    PageDataT        sensorPhi;
    Task             drawerTask;
    ClampMeterDriver clampMeter;

    std::shared_ptr<MenuModel<Keyboard>> menu;
};
