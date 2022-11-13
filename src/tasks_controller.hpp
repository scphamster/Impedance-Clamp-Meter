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
      , clampMeter{ valueOne, valueTwo, valueThree, valueFour, drawer.CreateAndGetDialog() }
      , valueOne{ std::make_shared<UniversalSafeType>(static_cast<float>(0)) }     // test
      , valueTwo{ std::make_shared<UniversalSafeType>(static_cast<float>(0)) }     // test
      , valueThree{ std::make_shared<UniversalSafeType>(static_cast<float>(0)) }   // test
      , valueFour{ std::make_shared<UniversalSafeType>(static_cast<float>(0)) }    // test
      , sensorPhi{ std::make_shared<UniversalSafeType>(static_cast<float>(0)) }    // test
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
        while (true) {
            drawer.DrawerTask();
            Task::DelayMsUntil(ProjectConfigs::DisplayDrawingDrawingPeriodMs);
        }
    }

    // initializers
    void InitializeMenu() noexcept
    {
        // measurements menu
        auto vout_info = std::make_shared<Page>(menu);
        vout_info->SetName("V out");
        vout_info->SetData(valueOne);
        vout_info->SetIndex(0);

        auto z_overall = std::make_shared<Page>(menu);
        z_overall->SetName("Z Overall");
        z_overall->SetData(valueTwo);
        z_overall->SetIndex(1);

        auto z_clamp = std::make_shared<Page>(menu);
        z_clamp->SetName("Z Clamp");
        z_clamp->SetData(valueThree);
        z_clamp->SetIndex(2);

        auto degree_nocall = std::make_shared<Page>(menu);
        degree_nocall->SetName("Degree");
        degree_nocall->SetData(valueFour);
        degree_nocall->SetIndex(3);

        auto measurements_page = std::make_shared<Page>(menu);
        measurements_page->SetIndex(0);
        measurements_page->SetName("Measurement");
        measurements_page->InsertChild(vout_info);
        measurements_page->InsertChild(z_overall);
        measurements_page->InsertChild(z_clamp);
        measurements_page->InsertChild(degree_nocall);
        measurements_page->SetKeyCallback(Keyboard::ButtonName::F1, [this]() { clampMeter.StartNormalModeOperation(); });
        measurements_page->SetKeyCallback(Keyboard::ButtonName::F2, [this]() { clampMeter.Stop(); });
        measurements_page->SetKeyCallback(Keyboard::ButtonName::F3, [this]() { clampMeter.SwitchToNextSensor(); });
        measurements_page->SetKeyCallback(Keyboard::ButtonName::F4, [this]() { clampMeter.DisableAGC(); });
        measurements_page->SetKeyCallback(Keyboard::ButtonName::Left, [this]() { clampMeter.DecreaseGain(); });
        measurements_page->SetKeyCallback(Keyboard::ButtonName::Down, [this]() { clampMeter.IncreaseGain(); });
        // calibration menu

        auto start_calibration = std::make_shared<Page>(menu);
        start_calibration->SetName("Press F1 to start calibration");
        start_calibration->SetHeader("Calibration");
        start_calibration->SetIndex(0);

        auto valueI = std::make_shared<Page>(menu);
        valueI->SetName("Absolute");
        valueI->SetData(valueOne);
        valueI->SetIndex(1);

        auto valueQ = std::make_shared<Page>(menu);
        valueQ->SetName("Deviation^2");
        valueQ->SetData(valueTwo);
        valueQ->SetIndex(2);

        auto valueD = std::make_shared<Page>(menu);
        // test
        //        auto testname= std::to_string(4.1234f);
        //        valueD->SetName(std::move(testname));
        valueD->SetName("Gain");
        valueD->SetData(valueThree);
        valueD->SetIndex(3);

        auto valueDNocall = std::make_shared<Page>(menu);
        valueDNocall->SetName("Phase shift");
        valueDNocall->SetData(valueFour);
        valueDNocall->SetIndex(4);

        auto calibration_page = std::make_shared<Page>(menu);
        calibration_page->SetIndex(1);
        calibration_page->SetName("Calibration");
        calibration_page->InsertChild(start_calibration);
        calibration_page->InsertChild(valueI);
        calibration_page->InsertChild(valueQ);
        calibration_page->InsertChild(valueD);
        calibration_page->InsertChild(valueDNocall);
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
    PageDataT valueOne;
    PageDataT valueTwo;
    PageDataT valueThree;
    PageDataT valueFour;

    PageDataT        sensorPhi;
    Task             drawerTask;
    ClampMeterDriver clampMeter;

    std::shared_ptr<MenuModel<Keyboard>> menu;
};
