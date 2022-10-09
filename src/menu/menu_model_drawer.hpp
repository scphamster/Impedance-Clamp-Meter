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
#include "semaphore.hpp"
#include "clamp_meter_concepts.hpp"

#include "menu_model_item.hpp"
#include "menu_model.hpp"

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
class MenuModelDrawer {
  public:
    using MenuModelT = MenuModel<Keyboard>;

    MenuModelDrawer(std::shared_ptr<Mutex> new_mutex, std::unique_ptr<Drawer> &&new_drawer)
      : mutex{ new_mutex }
      , drawer{ std::forward<decltype(new_drawer)>(new_drawer) }
    { }

    void DrawerTask() noexcept;
    using DifferenceType = void;   // todo : to be implemented
    void SetModel(std::shared_ptr<MenuModelT> new_model) noexcept;

  protected:
    DifferenceType    GetCurrentAndStoredDataDifference() noexcept;
    MenuModelPageItem GetCurrentModelPageData() noexcept;
    void              StoreCurrentModelData() noexcept;

  private:
    MenuModelPageItem           storedPage;   // stored data to be compared to freshly checked data from model
    std::shared_ptr<MenuModelT> model;        // model with data to be drawn
    std::shared_ptr<Mutex>      mutex;
    std::unique_ptr<Drawer>     drawer;
};

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
void
MenuModelDrawer<Drawer, Keyboard>::DrawerTask() noexcept
{
    // todo: implement
    drawer->SetCursor({ 0, 0 });
    drawer->SetTextColor(COLOR_RED, COLOR_BLACK);
    drawer->Print("dupadupadupadupadupadupadupa", 1);
}

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
void
MenuModelDrawer<Drawer, Keyboard>::GetCurrentAndStoredDataDifference() noexcept
{
    auto differences_was_found{ true };
    auto current_page = model->GetCurrentItem();

    if (differences_was_found) {
        StoreCurrentModelData();
    }
}

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
void
MenuModelDrawer<Drawer, Keyboard>::StoreCurrentModelData() noexcept
{ }

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
void
MenuModelDrawer<Drawer, Keyboard>::SetModel(std::shared_ptr<MenuModelT> new_model) noexcept
{
    model = new_model;
}

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
MenuModelPageItem
MenuModelDrawer<Drawer, Keyboard>::GetCurrentModelPageData() noexcept
{
    // todo: implement

    
}
