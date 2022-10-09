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
    using MenuModelT       = MenuModel<Keyboard>;
    using EditorCursorPosT = MenuModelPageItem::EditorCursorPosT;

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
    std::vector<MenuModelPageItemData> storedDataOnPage;
    std::shared_ptr<MenuModelT>        model;   // model with data to be drawn
    std::shared_ptr<Mutex>             mutex;
    std::unique_ptr<Drawer>            drawer;

    MenuModelIndex   childSelectionIndex;
    bool             isSomeChildSelected{ false };
    bool             editCursorActive{ false };
    EditorCursorPosT edittingCursorPosition{ 0 };

    // todo: make more versatile
    int namesColumnXPos{ 0 };
    int valuesColumnXPos{ 160 };
    int pageHeaderYPos{ 50 };
    int pageHeaderXPos{ 100 };
    int firstValueYPos{ 100 };

    int fontSize          = 1;
    int fontOneLineHeight = 14;
    int displayWidth      = 320;
    int displayHeight     = 480;
};

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
void
MenuModelDrawer<Drawer, Keyboard>::DrawerTask() noexcept
{
    auto page_items = model->GetCurrentItem()->GetChildren();

    drawer->SetCursor({ 0, 0 });
    drawer->SetTextColor(COLOR_GREEN, COLOR_BLACK);
    drawer->Print(page_items.size(), 1);

    auto name = page_items.at(0)->GetData()->GetName();

    drawer->SetCursor({ 0, 30 });
    drawer->Print(name.c_str(), 1);

//
//    for (auto item_num = 0; const auto &item : page_items) {
//        drawer->SetCursor({ namesColumnXPos, firstValueYPos + fontOneLineHeight * item_num });
//        drawer->SetTextColor(COLOR_WHITE, COLOR_BLACK);
//        drawer->Print(item->GetData()->GetName().c_str(), fontSize);
//        drawer->SetCursor({ valuesColumnXPos, firstValueYPos + fontOneLineHeight * item_num });
//
//         todo: make use of different types stored in variant
//        drawer->Print(item->GetData()->template GetValue<int>(), fontSize);
//
//        item_num++;
//    }


}

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
void
MenuModelDrawer<Drawer, Keyboard>::GetCurrentAndStoredDataDifference() noexcept
{
    auto differences_was_found{ true };
    auto page_items = model->GetCurrentItem()->GetChildren();

    if (page_items->size() != storedDataOnPage.size())
        differences_was_found = true;
    else {
        for (auto item_num = 0; const auto &item : page_items) {
            if (*item->GetData() != storedDataOnPage.at(item_num)) {
                drawer->SetCursor(namesColumnXPos, firstValueYPos + fontOneLineHeight * item_num);
                drawer->SetTextColor(COLOR_WHITE, COLOR_BLACK);
                drawer->Print(item->GetName(), fontSize);
                drawer->SetCursor(valuesColumnXPos, firstValueYPos + fontOneLineHeight * item_num);

                // todo: make use of different types stored in variant
                drawer->Print(item->GetData()->template GetValue<int>(), fontSize);

                differences_was_found = true;
                break;
            }

            item_num++;
        }
    }

    if (differences_was_found) {
        StoreCurrentModelData();
    }
}

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
void
MenuModelDrawer<Drawer, Keyboard>::StoreCurrentModelData() noexcept
{
    storedDataOnPage.clear();

    std::for_each(model->GetCurrentItem()->GetChildren().begin(),
                  model->GetCurrentItem()->GetChildren()->end(),
                  [&storage = this->storedDataOnPage](const auto &page_item) { storage.emplace_back(page_item); });
}

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
