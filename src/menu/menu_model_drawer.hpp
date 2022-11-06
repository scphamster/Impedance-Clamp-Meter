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

#include "menu_model.hpp"
#include "menu_model_item.hpp"
#include "menu_model_dialog.hpp"

#include "ili9486_config.h"

class ValueCursor {
  public:
    void Set(int new_position, bool activate_edit_cursor = true) noexcept
    {
        position = new_position;
        state    = activate_edit_cursor;
    }
    void               Activate(bool if_activate = true) noexcept { state = if_activate; }
    [[nodiscard]] int  GetPos() const noexcept { return position; }
    [[nodiscard]] bool IsActive() const noexcept { return state; }
    void               Increment() noexcept { position++; }
    void               Decrement() noexcept { position--; }

  private:
    int  position{ 0 };
    bool state{ false };
};

class ItemCursor {
  public:
    void               Set(int new_item_num, bool activate_edit_cursor = true) noexcept;
    void               Activate(bool if_activate = true) noexcept;
    [[nodiscard]] int  GetPos() const noexcept;
    [[nodiscard]] bool IsActive() const noexcept;
    void               Increment() noexcept
    {
        if (itemNum == maxVal)
            itemNum = minVal;
        else
            itemNum++;
    }
    void Decrement() noexcept
    {
        if (itemNum == minVal)
            itemNum = maxVal;
        else
            itemNum--;
    }
    void SetMinMax(size_t min, size_t max)
    {
        if (max < min)
            return;

        minVal = min;
        maxVal = max;
    }

  private:
    size_t itemNum{ 0 };
    size_t maxVal{ 0 };
    size_t minVal{ 0 };
    bool   itemCursorState{ false };
};

class PageCursor {
  public:
    enum class State {
        NotActive = 0,
        ItemLevel,
        ValueLevel
    };

    ValueCursor &GetValueCursor() noexcept { return valueCursor; }
    ItemCursor  &GetItemCursor() noexcept { return itemCursor; }
    State        GetState() const noexcept
    {
        if (itemCursor.IsActive()) {
            if (valueCursor.IsActive())
                return State::ValueLevel;
            else
                return State::ItemLevel;
        }
        else {
            return State::NotActive;
        }
    }
    bool operator==(const PageCursor &rhs) const noexcept
    {
        if ((valueCursor.GetPos() == rhs.valueCursor.GetPos()) and (itemCursor.GetPos() == rhs.itemCursor.GetPos()) and
            (GetState() == rhs.GetState())) {
            return true;
        }
        else
            return false;
    }

    void Reset() noexcept
    {
        valueCursor.Set(0, false);
        itemCursor.Set(0, false);
    }

  private:
    ValueCursor valueCursor;
    ItemCursor  itemCursor;
    State       activationState{ State::NotActive };
};

template<KeyboardC Keyboard>
class PageItemStateStorage {
  public:
    using Item  = MenuModelPage<Keyboard>;
    using DataT = UniversalType;

    [[nodiscard]] typename Item::NameT GetName() const noexcept { return name; }
    [[nodiscard]] DataT                GetValue() const noexcept { return value; }

    void SetName(const typename Item::NameT &new_name) noexcept { name = new_name; }
    void SetValue(const DataT &new_value) noexcept { value = new_value; }
    void SetValue(DataT &&new_value) noexcept { value = std::move(new_value); }

  private:
    typename Item::NameT name;
    DataT                value;
};

// todo: implement elegant fullRedraw flag
template<DisplayDrawerC Drawer, KeyboardC Keyboard>
class MenuModelDrawer {
  public:
    using Item        = MenuModelPage<Keyboard>;
    using NameT       = typename Item::NameT;
    using MenuModelT  = MenuModel<Keyboard>;
    using ColorT      = typename Drawer::ColorT;
    using ButtonName  = typename Keyboard::ButtonName;
    using ButtonEvent = typename Keyboard::ButtonEvent;
    using Point       = typename Drawer::Point;

    explicit MenuModelDrawer(std::unique_ptr<Drawer> &&new_drawer, std::unique_ptr<Keyboard> &&new_keyboard)
      : drawer{ std::forward<decltype(new_drawer)>(new_drawer) }
      , keyboard{ std::forward<decltype(new_keyboard)>(new_keyboard) }
    {
        //        keyboard->SetButtonEventCallback(Keyboard::ButtonName::Enter, Keyboard::ButtonEvent::Release, [this]() {
        //            EnterButtonPushEvent();
        //        });
        keyboard->SetButtonEventCallback(Keyboard::ButtonName::Back, Keyboard::ButtonEvent::Release, [this]() {
            BackButtonPushEvent();
        });
        keyboard->SetButtonEventCallback(Keyboard::ButtonName::Up, Keyboard::ButtonEvent::Release, [this]() {
            UpButtonReleaseEvent();
        });

        keyboard->SetMasterCallback([this](ButtonEvent event, ButtonName name) { KeyboardMasterCallback(event, name); });
    }

    void DrawerTask() noexcept;
    void SetModel(std::shared_ptr<MenuModelT> new_model) noexcept
    {
        model = new_model;
        ModelCurrentItemChangedEvent();
    }
    void ShowDialog(std::shared_ptr<MenuModelDialog> new_dialog) noexcept
    {
        dialogBox = std::move(new_dialog);
        dialogBox->SetIsShown(true);

        RequestFullRedraw();
    }

    [[nodiscard]] std::shared_ptr<MenuModelDialog> CreateAndGetDialog() noexcept
    {
        auto new_dialog = std::make_shared<MenuModelDialog>();
        new_dialog->SetOnShowCallback([this, new_dialog]() { this->ShowDialog(new_dialog); });
        return new_dialog;
    }

  protected:
    enum class PageItemDrawingMode {
        OnlyValue = 0,
        OnlyName,
        WholeItem
    };
    enum class ItemSelection : bool {
        Selected    = true,
        NotSelected = false
    };

    Item GetCurrentModelPageData() noexcept;
    void StoreCurrentModelData() noexcept;
    void DrawPageItemName(auto item_index, const auto &name) noexcept
    {
        ColorT back_color;
        if (cursor.GetState() == PageCursor::State::ItemLevel and cursor.GetItemCursor().GetPos() == item_index) {
            back_color = selectedItemBackground;
        }
        else {
            back_color = nonSelectedItemBackground;
        }

        drawer->SetCursor({ namesColumnXPos, firstValueYPos + item_index * itemsFontHeight });
        drawer->SetTextColor(itemNameColor, back_color);
        drawer->Print(name, itemsFontSize);
    }
    void DrawPageItemValue(auto item_index, auto const &value) noexcept
    {
        using State = typename PageCursor::State;
        ColorT back_color{};
        if (cursor.GetItemCursor().GetPos() == item_index and
            (cursor.GetState() == State::ItemLevel or cursor.GetState() == State::ValueLevel)) {
            back_color = selectedItemBackground;
        }
        else {
            back_color = nonSelectedItemBackground;
        }

        drawer->SetCursor({ valuesColumnXPos, firstValueYPos + item_index * itemsFontHeight });
        drawer->SetTextColor(itemValueColor, back_color);
        std::visit([this](auto &&printable_data) { drawer->Print(printable_data, itemsFontSize); }, value);
    }
    void DrawSinglePageItem(int item_index, PageItemDrawingMode mode) noexcept
    {
        if (mode == PageItemDrawingMode::WholeItem) {
            ColorT back_color{};

            if (cursor.GetState() == PageCursor::State::ItemLevel and cursor.GetItemCursor().GetPos() == item_index) {
                back_color = selectedItemBackground;
            }
            else {
                back_color = nonSelectedItemBackground;
            }

            drawer->DrawFiledRectangle({ screenUsedAreaLeftX, firstValueYPos + item_index * itemsFontHeight },
                                       screenUsedAreaRightX - screenUsedAreaLeftX,
                                       itemsFontHeight,
                                       back_color);
        }

        if (mode == PageItemDrawingMode::WholeItem or mode == PageItemDrawingMode::OnlyName) {
            DrawPageItemName(item_index, model->GetCurrentItem()->GetChild(item_index)->GetName());
        }

        if (mode == PageItemDrawingMode::WholeItem or mode == PageItemDrawingMode::OnlyValue) {
            if (model->GetCurrentItem()->GetChild(item_index)->IsValueless())
                return;

            DrawPageItemValue(item_index, model->GetCurrentItem()->GetChild(item_index)->GetData()->GetValue());
        }
    }
    void DrawStaticPageItems() const noexcept
    {
        drawer->DrawFiledRectangle({ screenUsedAreaLeftX, screenUsedAreaTopY },
                                   screenUsedAreaRightX - screenUsedAreaLeftX,
                                   screenUsedAreaBotY - screenUsedAreaTopY,
                                   COLOR_BLACK);   // todo: optimize

        drawer->SetCursor({ pageHeaderXPos, pageHeaderYPos });
        drawer->SetTextColor(COLOR_WHITE, COLOR_BLACK);
        if (currentPage->HasHeader())
            drawer->Print(currentPage->GetHeader(), pageNameFontSize);
        else
            drawer->Print(currentPage->GetName(), pageNameFontSize);
    }
    void DrawDynamicPageItems() noexcept
    {
        if (drawnDynamicPageItems.size() != model->GetCurrentItem()->GetChildren().size())
            drawnDynamicPageItems.resize(model->GetCurrentItem()->GetChildren().size());

        if (prevCursor != cursor) {
            if (cursor.GetState() != prevCursor.GetState()) {
                DrawSinglePageItem(cursor.GetItemCursor().GetPos(), PageItemDrawingMode::WholeItem);
            }
            else if (cursor.GetItemCursor().GetPos() != prevCursor.GetItemCursor().GetPos()) {
                DrawSinglePageItem(cursor.GetItemCursor().GetPos(), PageItemDrawingMode::WholeItem);
                DrawSinglePageItem(prevCursor.GetItemCursor().GetPos(), PageItemDrawingMode::WholeItem);
            }
            prevCursor = cursor;
        }

        for (auto item_index = 0; const auto &child_page : model->GetCurrentItem()->GetChildren()) {
            if (drawnDynamicPageItems.at(item_index).GetName() != child_page->GetName()) {
                drawnDynamicPageItems.at(item_index).SetName(child_page->GetName());
                DrawPageItemName(item_index, drawnDynamicPageItems.at(item_index).GetName());
            }

            if (not child_page->IsValueless()) {
                if (drawnDynamicPageItems.at(item_index).GetValue() != child_page->GetData()->GetValue()) {
                    drawnDynamicPageItems.at(item_index).SetValue(child_page->GetData()->GetValue());
                    DrawPageItemValue(item_index, child_page->GetData()->GetValue());
                }
            }

            item_index++;
        }
    }

    void RequestFullRedraw() noexcept
    {
        drawnDynamicPageItems.clear();
        if (dialogBox)
            dialogBox->SetHasBeenDrawnFlag(false);
        fullRedraw = true;
    }

    void DrawMessageDialog() noexcept
    {
        if (not dialogBox)
            return;

        if (not dialogBox->IsShown())
            return;

        if (dialogBox->HasValue() and dialogBox->ValueNeedsToBeRedrawn()) {
            drawer->SetCursor(dialogSettings.valuePosition);
            drawer->SetTextColor(dialogSettings.foreground, dialogSettings.background);

            std::visit([this](auto &&printable_data) { drawer->Print(printable_data, dialogSettings.fontSize); },
                       dialogBox->GetValue()->GetValue());
        }

        if (dialogBox->HasBeenDrawn())
            return;

        drawer->DrawFiledRectangle(dialogSettings.topLeft, 320, 200, dialogSettings.background);
        drawer->SetCursor(dialogSettings.topLeft);
        drawer->SetTextColor(dialogSettings.foreground, dialogSettings.background);
        drawer->Print(dialogBox->GetMessage(), dialogSettings.fontSize);

        dialogBox->SetHasBeenDrawnFlag(true);
    }

    void SetValueToInputBoxTEST() noexcept
    {
        *(dialogBox->GetValue()) = 36.3f;
        //        configASSERT(0);
    }

    // slots:

    void DialogBoxKeyboardHandler(ButtonEvent event, ButtonName button) noexcept
    {
        if (dialogBox->GetDialogType() == MenuModelDialog::DialogType::InputBox) {
            SetValueToInputBoxTEST();
        }

        if (button == ButtonName::Enter)
            dialogBox->KeyboardHandler(MenuModelDialog::Key::Enter);
        else if (button == ButtonName::Back)
            dialogBox->KeyboardHandler(MenuModelDialog::Key::Back);

        RequestFullRedraw();
    }

    void KeyboardMasterCallback(ButtonEvent event, ButtonName button) noexcept
    {
        if (event != ButtonEvent::Release)
            return;

        if (dialogBox)
            if (dialogBox->IsShown()) {
                DialogBoxKeyboardHandler(event, button);
                return;
            }

        model->GetCurrentItem()->InvokeSpecialKeyboardCallback(button);

        switch (button) {
        case ButtonName::Enter: EnterButtonPushEvent(); break;
        default: return;
        }
    }
    void ModelCurrentItemChangedEvent() noexcept
    {
        cursor.Reset();
        cursor.GetItemCursor().SetMinMax(0, model->GetCurrentItem()->GetChildCount() - 1);
    }
    void EnterButtonPushEvent() noexcept
    {
        if (cursor.GetItemCursor().IsActive()) {
            if (cursor.GetValueCursor().IsActive()) {
                cursor.GetValueCursor().Activate(false);
            }
            else {
                // check if item is editable
                if (model->GetCurrentItem()->GetChild(cursor.GetItemCursor().GetPos())->IsEditable())
                    cursor.GetValueCursor().Activate();
                else {
                    if (model->GetCurrentItem()->HasChild()) {
                        model->SetCurrentItem(model->GetCurrentItem()->GetChild(cursor.GetItemCursor().GetPos()));
                        ModelCurrentItemChangedEvent();
                    }
                }
            }
        }
        else {
            cursor.GetItemCursor().Activate();
        }
    }
    void BackButtonPushEvent() noexcept
    {
        if (cursor.GetItemCursor().IsActive()) {
            if (cursor.GetValueCursor().IsActive()) {
                // save changes
                cursor.GetValueCursor().Activate(false);
            }
            else {
                cursor.GetItemCursor().Activate(false);
            }
        }
        else {
            model->SetCurrentItem(model->GetCurrentItem()->GetParent());
            ModelCurrentItemChangedEvent();
        }
    }
    void LeftButtonReleaseEvent() noexcept
    {
        //        if (cursor.GetItemCursor().IsActive()) { }
    }
    void RightButtonReleaseEvent() noexcept { }
    void UpButtonReleaseEvent() noexcept
    {
        if (cursor.GetItemCursor().IsActive()) {
            if (cursor.GetValueCursor().IsActive()) { }
            else {
                cursor.GetItemCursor().Decrement();
            }
        }
    }

  private:
    std::vector<PageItemStateStorage<Keyboard>> drawnDynamicPageItems;
    std::shared_ptr<MenuModelT>                 model;   // model with data to be drawn
                                                         //    std::shared_ptr<Mutex>                      mutex;
    std::unique_ptr<Drawer> drawer;

    std::unique_ptr<Keyboard> keyboard;

    std::shared_ptr<Item> currentPage;

    PageCursor cursor;
    PageCursor prevCursor;

    // todo: use queue to make stream of dialogs
    std::shared_ptr<MenuModelDialog> dialogBox;

    bool staticPageItemsDrawn{ false };
    bool fullRedraw{ false };
    // todo: make more versatile
    // display sizes section
    int static constexpr displayHeight        = 480;
    int static constexpr displayWidth         = 320;
    int static constexpr screenUsedAreaLeftX  = 0;
    int static constexpr screenUsedAreaRightX = 319;
    int static constexpr screenUsedAreaTopY   = 80;
    int static constexpr screenUsedAreaBotY   = 430;

    int static constexpr itemsFontSize   = 1;
    int static constexpr itemsFontHeight = 20;

    // page items section
    int static constexpr firstValueYPos{ screenUsedAreaTopY + 100 };
    int static constexpr namesColumnXPos{ screenUsedAreaLeftX + 0 };
    int static constexpr valuesColumnXPos{ screenUsedAreaLeftX + 160 };

    // page header section
    int static constexpr pageHeaderYPos{ screenUsedAreaTopY + 0 };
    int static constexpr pageHeaderXPos{ screenUsedAreaLeftX + 60 };
    int static constexpr pageNameFontSize = 2;

    // page dialog message section
    struct DialogSettings {
        Point  topLeft;
        Point  bottRight;
        Point  valuePosition;
        int    fontSize;
        ColorT foreground, background;
    };

    DialogSettings static constexpr dialogSettings{ { screenUsedAreaLeftX + 0, screenUsedAreaTopY + 100 },
                                                    {},
                                                    { screenUsedAreaLeftX + 50, screenUsedAreaTopY + 150 },
                                                    1,
                                                    COLOR_REDYELLOW,
                                                    COLOR_DARKDARKGREY };

    //    int static constexpr msgDialogYpos{ screenUsedAreaTopY + 100 };
    //    int static constexpr msgDialogXPos{ screenUsedAreaLeftX + 0 };
    //    int static constexpr dialogSettings.fontSize{ 1 };
    //    int static constexpr msgDialogBackground{ COLOR_DARKDARKGREY };
    //    int static constexpr msgDialogForeground{ COLOR_REDYELLOW };

    ColorT static constexpr itemValueColor            = COLOR_DARKCYAN;
    ColorT static constexpr itemNameColor             = COLOR_DARKGREEN;
    ColorT static constexpr selectedItemBackground    = COLOR_DARKDARKGREY;
    ColorT static constexpr nonSelectedItemBackground = COLOR_BLACK;
};

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
void
MenuModelDrawer<Drawer, Keyboard>::DrawerTask() noexcept
{
    if (currentPage != model->GetCurrentItem() or fullRedraw) {
        staticPageItemsDrawn = false;
        currentPage          = model->GetCurrentItem();

        DrawStaticPageItems();
        staticPageItemsDrawn = true;

        drawnDynamicPageItems.clear();
    }
    // todo: draw message box
    DrawDynamicPageItems();
    DrawMessageDialog();

    fullRedraw = false;
}

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
void
MenuModelDrawer<Drawer, Keyboard>::StoreCurrentModelData() noexcept
{
    drawnDynamicPageItems.clear();

    std::for_each(model->GetCurrentItem()->GetChildren().begin(),
                  model->GetCurrentItem()->GetChildren()->end(),
                  [&storage = this->drawnDynamicPageItems](const auto &page_item) { storage.emplace_back(page_item); });
}

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
MenuModelPage<Keyboard>
MenuModelDrawer<Drawer, Keyboard>::GetCurrentModelPageData() noexcept
{
    // todo: implement
}
