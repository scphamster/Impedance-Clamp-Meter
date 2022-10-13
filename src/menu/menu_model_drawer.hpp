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

template<KeyboardC Keyboard>
class PageItemStateStorage {
  public:
    using Item = MenuModelPageItem<Keyboard>;

    [[nodiscard]] typename Item::NameT GetName() const noexcept { return name; }
    [[nodiscard]] UniversalType        GetValue() const noexcept { return value; }

    void SetName(const typename Item::NameT &new_name) noexcept { name = new_name; }
    void SetValue(const UniversalType &new_value) noexcept { value = new_value; }
    void SetValue(UniversalType &&new_value) noexcept { value = std::move(new_value); }

  private:
    typename Item::NameT name;
    UniversalType        value;
};

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
class MenuModelDrawer {
  public:
    using Item  = MenuModelPageItem<Keyboard>;
    using NameT = typename Item::NameT;

    using MenuModelT       = MenuModel<Keyboard>;
    using EditorCursorPosT = typename Item::EditorCursorPosT;

    explicit MenuModelDrawer(std::shared_ptr<Mutex>      new_mutex,
                             std::unique_ptr<Drawer>   &&new_drawer,
                             std::unique_ptr<Keyboard> &&new_keyboard)
      : mutex{ std::move(new_mutex) }
      , drawer{ std::forward<decltype(new_drawer)>(new_drawer) }
      , keyboard{ std::forward<decltype(new_keyboard)>(new_keyboard) }
    {
        keyboard->SetButtonEventCallback(Keyboard::ButtonName::Enter, Keyboard::ButtonEvent::Release, [this]() {
            EnterButtonPushEvent();
        });
    }

    void DrawerTask() noexcept;
    using DifferenceType = void;   // todo : to be implemented
    void SetModel(std::shared_ptr<MenuModelT> new_model) noexcept { model = new_model; }

    void PrintTestValue(int value) const noexcept
    {
        drawer->SetCursor({ 0, 0 });
        drawer->SetTextColor(COLOR_GREEN, COLOR_BLACK);
        drawer->Print(value, 1);
    }

  protected:
    Item GetCurrentModelPageData() noexcept;
    void StoreCurrentModelData() noexcept;
    void TestFunction() const noexcept;
    void DrawStaticPageItems() const noexcept;
    void DrawDynamicPageItems() noexcept;

    // slots:
    void EnterButtonPushEvent() noexcept { model->GetCurrentItem()->GetChild(0)->SetName("DUPA"); }

  private:
    std::vector<PageItemStateStorage<Keyboard>> drawnPageItems;
    std::shared_ptr<MenuModelT>                 model;   // model with data to be drawn
    std::shared_ptr<Mutex>                      mutex;
    std::unique_ptr<Drawer>                     drawer;
    std::unique_ptr<Keyboard>                   keyboard;

    std::shared_ptr<Item> currentPage;
    //    std::unique_ptr<Keyboard> keyboard;
    bool staticPageItemsDrawn{ false };

    MenuModelIndex   childSelectionIndex{};
    bool             isSomeChildSelected{ false };
    bool             editCursorActive{ false };
    EditorCursorPosT edittingCursorPosition{ 0 };
    // todo: make more versatile
    // display sizes section
    const int displayHeight        = 480;
    const int displayWidth         = 320;
    const int screenUsedAreaLeftX  = 0;
    const int screenUsedAreaRightX = 319;
    const int screenUsedAreaTopY   = 80;
    const int screenUsedAreaBotY   = 430;

    const int itemsFontSize   = 1;
    const int itemsFontHeight = 14;

    // page items section
    const int firstValueYPos{ screenUsedAreaTopY + 100 };
    const int namesColumnXPos{ screenUsedAreaLeftX + 0 };
    const int valuesColumnXPos{ screenUsedAreaLeftX + 160 };

    // page header section
    const int pageHeaderYPos{ screenUsedAreaTopY + 0 };
    const int pageHeaderXPos{ screenUsedAreaLeftX + 60 };
    const int pageNameFontSize = 2;
};

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
void
MenuModelDrawer<Drawer, Keyboard>::DrawerTask() noexcept
{
    if (currentPage != model->GetCurrentItem()) {
        staticPageItemsDrawn = false;
        currentPage          = model->GetCurrentItem();

        DrawStaticPageItems();
        staticPageItemsDrawn = true;

        drawnPageItems.clear();
    }

    DrawDynamicPageItems();
}

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
void
MenuModelDrawer<Drawer, Keyboard>::StoreCurrentModelData() noexcept
{
    drawnPageItems.clear();

    std::for_each(model->GetCurrentItem()->GetChildren().begin(),
                  model->GetCurrentItem()->GetChildren()->end(),
                  [&storage = this->drawnPageItems](const auto &page_item) { storage.emplace_back(page_item); });
}

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
MenuModelPageItem<Keyboard>
MenuModelDrawer<Drawer, Keyboard>::GetCurrentModelPageData() noexcept
{
    // todo: implement
}

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
void
MenuModelDrawer<Drawer, Keyboard>::TestFunction() const noexcept
{
    auto page_items = model->GetCurrentItem()->GetChildren();

    drawer->SetCursor({ 0, 0 });
    drawer->SetTextColor(COLOR_GREEN, COLOR_BLACK);
    drawer->Print(page_items.size(), 1);

    drawer->SetCursor({ 0, 30 });
    drawer->Print(page_items.at(0)->GetData()->GetName(), 1);

    std::visit([this](auto &&value) { drawer->Print(value, 1); }, *page_items.at(0)->GetData()->GetValue());
}

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
void
MenuModelDrawer<Drawer, Keyboard>::DrawStaticPageItems() const noexcept
{
    drawer->DrawFiledRectangle({ screenUsedAreaLeftX, screenUsedAreaTopY },
                               screenUsedAreaRightX - screenUsedAreaLeftX,
                               screenUsedAreaBotY - screenUsedAreaTopY,
                               COLOR_BLACK);   // todo: optimize

    drawer->SetCursor({ pageHeaderXPos, pageHeaderYPos });
    drawer->SetTextColor(COLOR_WHITE, COLOR_BLACK);
    drawer->Print(currentPage->GetName(), pageNameFontSize);
}

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
void
MenuModelDrawer<Drawer, Keyboard>::DrawDynamicPageItems() noexcept
{
    if (drawnPageItems.size() != model->GetCurrentItem()->GetChildren().size())
        drawnPageItems.resize(model->GetCurrentItem()->GetChildren().size());

    auto page_item_name_drawer = [this](auto item_index, const auto &value) {
        drawer->SetCursor({ namesColumnXPos, firstValueYPos + item_index * itemsFontHeight });
        drawer->SetTextColor(COLOR_DARKGREEN, COLOR_BLACK);

        drawer->Print(value, itemsFontSize);
    };

    auto page_item_value_drawer = [this](auto item_index, const auto &value) {
        drawer->SetCursor({ valuesColumnXPos, firstValueYPos + item_index * itemsFontHeight });
        drawer->SetTextColor(COLOR_CYAN, COLOR_BLACK);
        std::visit([this](auto &&printable_data) { drawer->Print(printable_data, itemsFontSize); }, *value);
    };

    for (auto item_index = 0; const auto &child_page : model->GetCurrentItem()->GetChildren()) {
        if (drawnPageItems.at(item_index).GetName() != child_page->GetName()) {
            drawnPageItems.at(item_index).SetName(child_page->GetName());
            page_item_name_drawer(item_index, drawnPageItems.at(item_index).GetName());
        }
        //        page_item_name_drawer(item_index, drawnPageItems.at(item_index).GetName());

        if (drawnPageItems.at(item_index).GetValue() != *child_page->GetData().GetValue()) {
            drawnPageItems.at(item_index).SetValue(*child_page->GetData().GetValue());
            page_item_value_drawer(item_index, child_page->GetData().GetValue());
        }
        //        page_item_value_drawer(item_index, child_page->GetData()->GetValue());
    }
}
