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

//todo: move to enum
#define COLOR_BLACK        0x0000 /*   0,   0,   0 */
#define COLOR_NAVY         0x000F /*   0,   0, 128 */
#define COLOR_DARKGREEN    0x03E0 /*   0, 128,   0 */
#define COLOR_DDGREEN      0xc0
#define COLOR_DARKCYAN     0x03EF /*   0, 128, 128 */
#define COLOR_MAROON       0x7800 /* 128,   0,   0 */
#define COLOR_PURPLE       0x780F /* 128,   0, 128 */
#define COLOR_OLIVE        0x7BE0 /* 128, 128,   0 */
#define COLOR_LIGHTGREY    0xC618 /* 192, 192, 192 */
#define COLOR_DARKGREY     0x7BEF /* 128, 128, 128 */
#define COLOR_BLUE         0x001F /*   0,   0, 255 */
#define COLOR_GREEN        0x07E0 /*   0, 255,   0 */
#define COLOR_CYAN         0x07FF /*   0, 255, 255 */
#define COLOR_RED          0xF800 /* 255,   0,   0 */
#define COLOR_MAGENTA      0xF81F /* 255,   0, 255 */
#define COLOR_YELLOW       0xFFE0 /* 255, 255,   0 */
#define COLOR_WHITE        0xFFFF /* 255, 255, 255 */
#define COLOR_ORANGE       0xFD20 /* 255, 165,   0 */
#define COLOR_GREENYELLOW  0xAFE5 /* 173, 255,  47 */
#define COLOR_PINK         0xF81F
#define COLOR_DARKDARKGREY 0x514A   // 10,10,10

#define COLOR_REDYELLOW 60580
#define COLOR_LIGHTMUD  42023
#define COLOR_GREY      10565

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

    ValueCursor        &GetValueCursor() noexcept { return valueCursor; }
    ItemCursor         &GetPageCursor() noexcept { return itemCursor; }
    [[nodiscard]] State GetState() const noexcept
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

    void SetName(typename Item::NameT const &new_name) noexcept { name = new_name; }
    void SetValue(DataT const &new_value) noexcept { value = new_value; }
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
    using Rect        = typename Drawer::Rect;
    using ScreenSizeT = typename Drawer::ScreenSizeT;

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

    // todo: make this function a real freertos task, not just a function called from outside
    void DrawerTask() noexcept
    {
        if (currentPage != model->GetCurrentPage() or fullRedraw) {
            staticPageItemsDrawn = false;
            currentPage          = model->GetCurrentPage();

            DrawStaticPageItems();
            staticPageItemsDrawn = true;

            drawnDynamicPageItems.clear();
        }
        // todo: draw message box
        DrawDynamicPageItems();
        DrawMessageDialog();

        fullRedraw = false;
    }
    void SetModel(std::shared_ptr<MenuModelT> new_model) noexcept
    {
        model = new_model;
        ModelCurrentItemChangedEvent();
    }
    void ShowDialog(std::shared_ptr<MenuModelDialog> new_dialog) noexcept
    {
        dialogBox = std::move(new_dialog);
        dialogBox->SetIsShown(true);

        //        RequestFullRedraw();
        RequestMessageBoxRedraw();
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
        if (cursor.GetState() == PageCursor::State::ItemLevel and cursor.GetPageCursor().GetPos() == item_index) {
            back_color = selectedItemBackground;
        }
        else {
            back_color = nonSelectedItemBackground;
        }

        // todo: add cleaning?
        drawer->SetCursor({ itemsSettings.namePos, itemsSettings.usedArea.top + item_index * itemsSettings.fontHeight });
        drawer->SetTextColor(itemsSettings.nameColor, back_color);
        drawer->Print(name, itemsSettings.fontSize);
    }
    void DrawPageItemValue(auto item_index, auto const &value) noexcept
    {
        using State = typename PageCursor::State;
        ColorT back_color{};
        if (cursor.GetPageCursor().GetPos() == item_index and
            (cursor.GetState() == State::ItemLevel or cursor.GetState() == State::ValueLevel)) {
            back_color = selectedItemBackground;
        }
        else {
            back_color = nonSelectedItemBackground;
        }

        drawer->DrawFiledRectangle(
          Point{ itemsSettings.valuePos, itemsSettings.usedArea.top + item_index * itemsSettings.fontHeight },
          itemsSettings.valueFieldWidth,
          itemsSettings.fontHeight,
          back_color);
        drawer->SetCursor({ itemsSettings.valuePos, itemsSettings.usedArea.top + item_index * itemsSettings.fontHeight });
        drawer->SetTextColor(itemsSettings.valueColor, back_color);
        std::visit([this](auto &&printable_data) { drawer->Print(printable_data, itemsSettings.fontSize); }, value);
    }
    void DrawSinglePageItem(int item_index, PageItemDrawingMode mode) noexcept
    {
        if (mode == PageItemDrawingMode::WholeItem) {
            ColorT back_color{};

            if (cursor.GetState() == PageCursor::State::ItemLevel and cursor.GetPageCursor().GetPos() == item_index) {
                back_color = selectedItemBackground;
            }
            else {
                back_color = nonSelectedItemBackground;
            }

            drawer->DrawFiledRectangle(
              Rect{ { itemsSettings.usedArea.left, itemsSettings.usedArea.top + item_index * itemsSettings.fontHeight },
                    { itemsSettings.usedArea.right, itemsSettings.usedArea.top + (item_index + 1) * itemsSettings.fontHeight } },
              back_color);
        }

        if (mode == PageItemDrawingMode::WholeItem or mode == PageItemDrawingMode::OnlyName) {
            DrawPageItemName(item_index, model->GetCurrentPage()->GetChild(item_index)->GetName());
        }

        if (mode == PageItemDrawingMode::WholeItem or mode == PageItemDrawingMode::OnlyValue) {
            if (model->GetCurrentPage()->GetChild(item_index)->IsValueless())
                return;

            DrawPageItemValue(item_index, model->GetCurrentPage()->GetChild(item_index)->GetData()->GetValue());
        }
    }
    void DrawStaticPageItems() const noexcept
    {
        drawer->DrawFiledRectangle(screenSettings.usedArea, COLOR_BLACK);   // todo: optimize

        drawer->SetCursor(pageHeaderSettings.position);
        drawer->SetTextColor(COLOR_WHITE, COLOR_BLACK);

        drawer->Print(currentPage->GetHeader(), pageHeaderSettings.fontSize);
    }
    void DrawDynamicPageItems() noexcept
    {
        if (drawnDynamicPageItems.size() != model->GetCurrentPage()->GetChildren().size())
            drawnDynamicPageItems.resize(model->GetCurrentPage()->GetChildren().size());

        if (prevCursor != cursor) {
            if (cursor.GetState() != prevCursor.GetState()) {
                DrawSinglePageItem(cursor.GetPageCursor().GetPos(), PageItemDrawingMode::WholeItem);
            }
            else if (cursor.GetPageCursor().GetPos() != prevCursor.GetPageCursor().GetPos()) {
                DrawSinglePageItem(cursor.GetPageCursor().GetPos(), PageItemDrawingMode::WholeItem);
                DrawSinglePageItem(prevCursor.GetPageCursor().GetPos(), PageItemDrawingMode::WholeItem);
            }
            prevCursor = cursor;
        }

        for (auto item_index = 0; const auto &child_page : model->GetCurrentPage()->GetChildren()) {
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

        if (dialogBox->HasBeenDrawn() and (not messageBoxShouldBeRedrawn))
            return;

        drawer->DrawFiledRectangle(dialogSettings.usedArea, dialogSettings.background);
        drawer->SetCursor(dialogSettings.usedArea.GetTopLeft());
        drawer->SetTextColor(dialogSettings.foreground, dialogSettings.background);
        drawer->Print(dialogBox->GetMessage(), dialogSettings.fontSize);

        dialogBox->SetHasBeenDrawnFlag(true);
        messageBoxShouldBeRedrawn = false;
    }
    void RequestFullRedraw() noexcept
    {
        if (dialogBox)
            dialogBox->SetHasBeenDrawnFlag(false);

        fullRedraw = true;
    }
    void RequestMessageBoxRedraw() noexcept { messageBoxShouldBeRedrawn = true; }
    // slots:
    void KeyboardMasterCallback(ButtonEvent event, ButtonName button) noexcept
    {
        if (event != ButtonEvent::Release)
            return;

        if (dialogBox)
            if (dialogBox->IsWaitingForUserInput()) {
                DialogBoxKeyboardHandler(event, button);
                return;
            }

        model->GetCurrentPage()->InvokeSpecialKeyboardCallback(button);

        switch (button) {
        case ButtonName::Enter: EnterButtonPushEvent(); break;
        default: return;
        }
    }
    void DialogBoxKeyboardHandler(ButtonEvent event, ButtonName button) noexcept
    {
        // todo: implement handling of input box

        if (button == ButtonName::Enter)
            dialogBox->KeyboardHandler(MenuModelDialog::Key::Enter);
        else if (button == ButtonName::Back)
            dialogBox->KeyboardHandler(MenuModelDialog::Key::Back);

        RequestFullRedraw();
    }
    void EnterButtonPushEvent() noexcept
    {
        if (cursor.GetPageCursor().IsActive()) {
            if (cursor.GetValueCursor().IsActive()) {
                cursor.GetValueCursor().Activate(false);
            }
            else {
                // if item is editable
                if (model->GetCurrentPage()->GetChild(cursor.GetPageCursor().GetPos())->IsEditable())
                    cursor.GetValueCursor().Activate();
                else {
                    if (model->GetCurrentPage()->HasChild()) {
                        model->SetCurrentItem(model->GetCurrentPage()->GetChild(cursor.GetPageCursor().GetPos()));
                        ModelCurrentItemChangedEvent();
                    }
                }
            }
        }
        else {
            cursor.GetPageCursor().Activate();
        }
    }
    void BackButtonPushEvent() noexcept
    {
        if (cursor.GetPageCursor().IsActive()) {
            if (cursor.GetValueCursor().IsActive()) {
                // save changes
                cursor.GetValueCursor().Activate(false);
            }
            else {
                cursor.GetPageCursor().Activate(false);
            }
        }
        else {
            model->SetCurrentItem(model->GetCurrentPage()->GetParent());
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
        if (cursor.GetPageCursor().IsActive()) {
            if (cursor.GetValueCursor().IsActive()) { }
            else {
                cursor.GetPageCursor().Decrement();
            }
        }
    }
    void ModelCurrentItemChangedEvent() noexcept
    {
        cursor.Reset();
        cursor.GetPageCursor().SetMinMax(0, model->GetCurrentPage()->GetChildCount() - 1);
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
    bool messageBoxShouldBeRedrawn{ false };
    // todo: make more versatile
    // display sizes section
    struct DisplaySettings {
        ScreenSizeT height, width;
        Rect        usedArea;
    };
    DisplaySettings static constexpr screenSettings{ 480, 320, { { 0, 80 }, { 320, 430 } } };

    // page items section
    struct PageItemsSettings {
        constexpr PageItemsSettings() = default;
        constexpr PageItemsSettings(Rect        used_area,
                                    ScreenSizeT name_offset,
                                    ScreenSizeT value_offset,
                                    ColorT      name_foreground,
                                    ColorT      value_foreground,
                                    ScreenSizeT font_size,
                                    ScreenSizeT font_height)
          : usedArea{ used_area }
          , namePos{ name_offset + used_area.left }
          , valuePos{ value_offset + used_area.left }
          , nameFieldWidth{ value_offset - name_offset }
          , valueFieldWidth{ used_area.right - (used_area.left + value_offset) }
          , fontSize{ font_size }
          , fontHeight{ 20 }
          , nameColor{ name_foreground }
          , valueColor{ value_foreground }
        { }

        Rect        usedArea;
        ScreenSizeT namePos{};
        ScreenSizeT valuePos{};
        ScreenSizeT nameFieldWidth{};
        ScreenSizeT valueFieldWidth{};
        ScreenSizeT fontSize{ 1 };
        ScreenSizeT fontHeight{ itemsSettings.fontHeight };
        ColorT      nameColor{ COLOR_WHITE };
        ColorT      valueColor{ COLOR_DARKCYAN };
    };
    PageItemsSettings static constexpr itemsSettings{ Rect{ screenSettings.usedArea.left,
                                                            screenSettings.usedArea.top + 100,
                                                            screenSettings.usedArea.right,
                                                            screenSettings.usedArea.bot - 200 },
                                                      0,
                                                      160,
                                                      COLOR_DARKGREEN,
                                                      COLOR_DARKCYAN,
                                                      1,
                                                      15 };

    struct PageHeader {
        Point  position;
        int    fontSize{ 1 };
        ColorT foreground, background;
    };
    PageHeader static constexpr pageHeaderSettings{ { screenSettings.usedArea.left + 40, screenSettings.usedArea.top + 60 },
                                                    2,
                                                    COLOR_GREY,
                                                    COLOR_BLACK };

    struct DialogSettings {
        constexpr DialogSettings() = default;
        constexpr DialogSettings(Rect used_area, Point value_position, int font_size, ColorT new_foreground, ColorT new_background)
          : usedArea{ used_area }
          , valuePosition{ used_area.left + value_position.x, used_area.top + value_position.y }
          , fontSize{ font_size }
          , foreground{ new_foreground }
          , background{ new_background }
        { }

        Rect   usedArea;
        Point  valuePosition;
        int    fontSize{ 1 };
        ColorT foreground, background;
    };
    DialogSettings static constexpr dialogSettings{
        Rect{ Point{ screenSettings.usedArea.left + 0, screenSettings.usedArea.top + 300 },
              Point{ screenSettings.usedArea.left + 319, screenSettings.usedArea.top + 400 } },
        Point{ 50, 50 },
        1,
        COLOR_REDYELLOW,
        COLOR_DARKDARKGREY
    };

    ColorT static constexpr selectedItemBackground    = COLOR_DARKDARKGREY;
    ColorT static constexpr nonSelectedItemBackground = COLOR_BLACK;
};

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
void
MenuModelDrawer<Drawer, Keyboard>::StoreCurrentModelData() noexcept
{
    drawnDynamicPageItems.clear();

    std::for_each(model->GetCurrentPage()->GetChildren().begin(),
                  model->GetCurrentPage()->GetChildren()->end(),
                  [&storage = this->drawnDynamicPageItems](const auto &page_item) { storage.emplace_back(page_item); });
}

template<DisplayDrawerC Drawer, KeyboardC Keyboard>
MenuModelPage<Keyboard>
MenuModelDrawer<Drawer, Keyboard>::GetCurrentModelPageData() noexcept
{
    // todo: implement
}
