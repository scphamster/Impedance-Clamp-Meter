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
#include <variant>
#include <vector>
#include <map>
#include <string>
#include <utility>
#include <functional>
// #include <shared_ptr>

#include "universal_type.hpp"
#include "clamp_meter_concepts.hpp"
#include "menu_model.hpp"

using MenuModelIndex = size_t;

class MenuModelPageItemData {
  public:
    using IntegerType = int;
    using FloatType   = float;
    using StringType  = std::string;

    MenuModelPageItemData(std::shared_ptr<UniversalType> new_value)
      : value{ std::move(new_value) }
    { }
    MenuModelPageItemData()
      : MenuModelPageItemData{ std::make_shared<UniversalType>() }
    { }

    void SetValue(std::shared_ptr<UniversalType> new_value) noexcept { value = std::move(new_value); }
    [[nodiscard]] std::shared_ptr<UniversalType> GetValue() const noexcept { return value; }
    bool                                         operator==(const auto &rhs)
    {
        if (value == rhs.value)
            return true;
        else
            return false;
    }

  private:
    std::shared_ptr<UniversalType> value;
};

template<KeyboardC Keyboard>
class MenuModelPageItem : public std::enable_shared_from_this<MenuModelPageItem<Keyboard>> {
  public:
    using ChildIndex       = int;
    using EditorCursorPosT = int;
    using NameT            = std::string;
    using Model            = MenuModel<Keyboard>;
    using ButtonId         = typename Keyboard::ButtonId;
    using CallbacksVector  = std::vector<std::pair<ButtonId, std::function<void()>>>;

    using std::enable_shared_from_this<MenuModelPageItem<Keyboard>>::shared_from_this;

    MenuModelPageItem(std::shared_ptr<MenuModel<Keyboard>> belongs_to_model) noexcept { model = belongs_to_model; }

    [[nodiscard]] std::shared_ptr<MenuModelPageItem> GetPtr() noexcept { return shared_from_this(); }
    [[nodiscard]] MenuModelPageItemData              GetData() const noexcept { return data; }
    [[nodiscard]] bool                               HasChild(MenuModelIndex at_position = 0) const noexcept
    {
        return (childItems.size() > 0) ? true : false;
    }
    [[nodiscard]] std::shared_ptr<MenuModelPageItem> GetChild(MenuModelIndex at_position) const noexcept
    {
        if (at_position >= childItems.size())
            return *(childItems.end() - 1);

        return childItems.at(at_position);
    }
    [[nodiscard]] std::vector<std::shared_ptr<MenuModelPageItem>> GetChildren() const noexcept { return childItems; }
    [[nodiscard]] auto             GetChildCount() const noexcept { return childItems.size(); }
    [[nodiscard]] MenuModelIndex   GetPosition() const noexcept { return residesAtIndex; }
    [[nodiscard]] MenuModelIndex   GetSelection() const noexcept { return childSelectionIndex; }
    [[nodiscard]] bool             SomeItemIsSelected() const noexcept { return isSomeChildSelected; }
    [[nodiscard]] EditorCursorPosT GetEdittingCursorPosition() const noexcept { return editingCursorPosition; }
    [[nodiscard]] bool             EdittingCursorIsActive() const noexcept { return editCursorActive; }
    [[nodiscard]] std::shared_ptr<MenuModelPageItem> GetParent() noexcept
    {
        if (parent.get() == nullptr)
            return This();
        else
            return parent;
    }
    [[nodiscard]] NameT GetName() noexcept { return name; }
    CallbacksVector     GetKeyboardCallbacks() const noexcept
    {
        constexpr auto number_of_callbacks = 5;
        auto           callbacks           = CallbacksVector{ std::pair{ 1, []() {} } };
    }
    bool IsEditable() const noexcept { return isEditable; }

    void SetData(const MenuModelPageItemData &new_data) noexcept { data = new_data; }
    void InsertChild(std::shared_ptr<MenuModelPageItem> child, MenuModelIndex at_position) noexcept
    {
        if (at_position < 0)
            return;

        child->SetParent(This());

        if (childItems.size() <= at_position) {
            child->SetIndex(childItems.size());
            childItems.emplace_back(std::move(child));
        }
        else {
            childItems.at(at_position) = std::move(child);
        }
    }
    void InsertChild(std::shared_ptr<MenuModelPageItem> child) noexcept { InsertChild(child, child->GetPosition()); }
    void SetIndex(MenuModelIndex idx) noexcept { residesAtIndex = idx; }
    void SetSelectionPosition(MenuModelIndex selected_item_position) noexcept
    {
        childSelectionIndex = selected_item_position;
    }
    void SetEditingCursorPosition(EditorCursorPosT new_position) noexcept { editingCursorPosition = new_position; }
    void ActivateEditingCursor(bool if_activate) noexcept { editCursorActive = if_activate; }
    void SetParent(std::shared_ptr<MenuModelPageItem> new_parent) noexcept { parent = new_parent; }
    void SetName(const NameT &new_name) noexcept { name = new_name; }
    void SetModifiability(bool if_editable = true) noexcept { isEditable = if_editable; }

  protected:
    auto This() noexcept { return GetPtr(); }

  private:
    NameT                                           name;
    std::shared_ptr<Model>                          model;
    MenuModelIndex                                  residesAtIndex{};
    MenuModelIndex                                  childSelectionIndex{};
    bool                                            isSomeChildSelected{ false };
    bool                                            editCursorActive{ false };
    EditorCursorPosT                                editingCursorPosition{ 0 };
    MenuModelPageItemData                           data;
    std::vector<std::shared_ptr<MenuModelPageItem>> childItems{};
    std::shared_ptr<MenuModelPageItem>              parent;
    bool                                            isEditable{ false };
};
