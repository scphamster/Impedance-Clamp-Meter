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

#include <mutex>
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
    using ValueType   = UniversalType;

    MenuModelPageItemData() = default;

    MenuModelPageItemData(ValueType new_value)
      : value{ std::move(new_value) }
    { }

    MenuModelPageItemData &operator=(const auto &new_value)
    {
        value = new_value;
        return *this;
    }

    void SetValue(ValueType new_value) noexcept
    {
        std::lock_guard<Mutex>{ mutex };
        value = std::move(new_value);
    }
    [[nodiscard]] ValueType GetValue() noexcept
    {
        std::lock_guard<Mutex>{ mutex };
        return value;
    }

    bool operator==(const auto &rhs)
    {
        std::lock_guard<Mutex>{ mutex };

        if (value == rhs.value)
            return true;
        else
            return false;
    }

  private:
    ValueType value;
    Mutex     mutex;
};

template<KeyboardC Keyboard>
class MenuModelPageItem : public std::enable_shared_from_this<MenuModelPageItem<Keyboard>> {
  public:
    using ChildIndex               = int;
    using EditorCursorPosT         = int;
    using NameT                    = std::string;
    using Model                    = MenuModel<Keyboard>;
    using ButtonId                 = typename Keyboard::ButtonId;
    using ButtonName               = typename Keyboard::ButtonName;
    using SpecialKeyboardCallback  = std::function<void()>;
    using SpecialKeyboardCallbacks = std::map<ButtonName, SpecialKeyboardCallback>;
    using DataT                    = std::shared_ptr<MenuModelPageItemData>;

    using std::enable_shared_from_this<MenuModelPageItem<Keyboard>>::shared_from_this;

    MenuModelPageItem(std::shared_ptr<MenuModel<Keyboard>> belongs_to_model) noexcept
      : model{ belongs_to_model }
    { }

    [[nodiscard]] std::shared_ptr<MenuModelPageItem> GetPtr() noexcept { return shared_from_this(); }
    [[nodiscard]] DataT                              GetData() const noexcept { return data; }
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
    [[nodiscard]] auto                               GetChildCount() const noexcept { return childItems.size(); }
    [[nodiscard]] MenuModelIndex                     GetPosition() const noexcept { return residesAtIndex; }
    [[nodiscard]] std::shared_ptr<MenuModelPageItem> GetParent() noexcept
    {
        if (not parent)
            return This();
        else
            return parent;
    }
    [[nodiscard]] NameT                    GetName() noexcept { return name; }
    [[nodiscard]] SpecialKeyboardCallbacks GetKeyboardCallbacks() const noexcept { return specialKeyCallbacks; }
    [[nodiscard]] bool                     IsEditable() const noexcept { return isEditable; }
    [[nodiscard]] bool                     IsValueless() const noexcept
    {
        if (not data)
            return true;
        else
            false;
    }

    void SetData(DataT new_data) noexcept { data = new_data; }
    void InsertChild(std::shared_ptr<MenuModelPageItem> child, MenuModelIndex at_idx) noexcept
    {
        child->SetParent(This());

        if (childItems.size() <= at_idx) {
            child->SetIndex(childItems.size());
            childItems.emplace_back(std::move(child));
        }
        else {
            childItems.at(at_idx) = std::move(child);
        }
    }
    void InsertChild(std::shared_ptr<MenuModelPageItem> child) noexcept { InsertChild(child, child->GetPosition()); }
    void SetIndex(MenuModelIndex idx) noexcept { residesAtIndex = idx; }
    void SetParent(std::shared_ptr<MenuModelPageItem> new_parent) noexcept { parent = new_parent; }
    void SetName(const NameT &new_name) noexcept { name = new_name; }
    void SetModifiability(bool if_editable = true) noexcept { isEditable = if_editable; }
    void SetKeyCallback(ButtonName for_button, SpecialKeyboardCallback &&new_callback) noexcept
    {
        specialKeyCallbacks[for_button] = std::move(new_callback);
    }
    void InvokeSpecialKeyboardCallback(ButtonName button)
    {
        if (specialKeyCallbacks.contains(button))
            specialKeyCallbacks.at(button)();
    }

  protected:
    auto This() noexcept { return GetPtr(); }

  private:
    std::shared_ptr<MenuModelPageItem>              parent;
    std::shared_ptr<Model>                          model;
    MenuModelIndex                                  residesAtIndex{0};
    NameT                                           name;
    NameT                                           pageHeader;
    DataT                                           data;
    std::vector<std::shared_ptr<MenuModelPageItem>> childItems{};
    bool                                            isEditable{ false };
    SpecialKeyboardCallbacks                        specialKeyCallbacks;
};
