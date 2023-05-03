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

template<KeyboardC Keyboard>
class MenuModelPage : public std::enable_shared_from_this<MenuModelPage<Keyboard>> {
  public:
    using std::enable_shared_from_this<MenuModelPage<Keyboard>>::shared_from_this;

    using ChildIndex               = int;
    using EditorCursorPosT         = int;
    using NameT                    = std::string;
    using HeaderT                  = NameT;
    using Model                    = MenuModel<Keyboard>;
    using ButtonId                 = typename Keyboard::ButtonId;
    using ButtonName               = typename Keyboard::ButtonName;
    using SpecialKeyboardCallback  = std::function<void()>;
    using SpecialKeyboardCallbacks = std::map<ButtonName, SpecialKeyboardCallback>;
    using DataT                    = std::shared_ptr<UniversalSafeType>;
    using EventCallback            = std::function<void()>;
    enum class Event {
        Entrance
    };
    using EventCallbacks = std::map<Event, EventCallback>;

    explicit MenuModelPage(std::shared_ptr<MenuModel<Keyboard>> belongs_to_model) noexcept
      : model{ belongs_to_model }
    { }

    [[nodiscard]] std::shared_ptr<MenuModelPage> GetPtr() noexcept { return shared_from_this(); }
    [[nodiscard]] DataT                          GetData() const noexcept { return data; }
    [[nodiscard]] bool HasChild(MenuModelIndex at_position = 0) const noexcept { return (childItems.size() > 0) ? true : false; }
    [[nodiscard]] std::shared_ptr<MenuModelPage> GetChild(MenuModelIndex at_position = 0) const noexcept
    {
        if (at_position >= childItems.size())
            return *(childItems.end() - 1);

        return childItems.at(at_position);
    }
    [[nodiscard]] std::vector<std::shared_ptr<MenuModelPage>> GetChildren() const noexcept { return childItems; }
    [[nodiscard]] auto                                        GetChildCount() const noexcept { return childItems.size(); }
    [[nodiscard]] MenuModelIndex                              GetPosition() const noexcept { return residesAtIndex; }
    [[nodiscard]] std::shared_ptr<MenuModelPage>              GetParent() noexcept
    {
        if (not parent)
            return This();
        else
            return parent;
    }
    [[nodiscard]] NameT                    GetName() noexcept { return name; }
    [[nodiscard]] SpecialKeyboardCallbacks GetKeyboardCallbacks() const noexcept { return specialKeyCallbacks; }
    [[nodiscard]] bool                     IsEditable() const noexcept { return isEditable; }
    [[nodiscard]] bool                     IsValueless() const noexcept { return (data) ? false : true; }
    [[nodiscard]] HeaderT                  GetHeader() const noexcept
    {
        if (pageHeader.empty())
            return name;
        else
            return pageHeader;
    }
    [[nodiscard]] bool HasHeader() const noexcept { return (pageHeader.empty()) ? false : true; }
    void               SetData(DataT new_data) noexcept { data = new_data; }
    void               InsertChild(std::shared_ptr<MenuModelPage> child, MenuModelIndex at_idx) noexcept
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
    void InsertChild(std::shared_ptr<MenuModelPage> child) noexcept { InsertChild(child, child->GetPosition()); }
    void SetIndex(MenuModelIndex idx) noexcept { residesAtIndex = idx; }
    void SetParent(std::shared_ptr<MenuModelPage> new_parent) noexcept { parent = new_parent; }
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
    void SetHeader(const HeaderT &new_header) noexcept { pageHeader = new_header; }
    void SetEventCallback(Event event, EventCallback &&callback) noexcept { eventCallbacks[event] = std::move(callback); }
    void InvokeEventCallback(Event event) noexcept
    {
        if (eventCallbacks.contains(event))
            eventCallbacks.at(event)();
    }

  protected:
    auto This() noexcept { return GetPtr(); }

  private:
    std::shared_ptr<MenuModelPage>              parent;
    std::shared_ptr<Model>                      model;
    MenuModelIndex                              residesAtIndex{ 0 };
    NameT                                       name;
    NameT                                       pageHeader;
    DataT                                       data;
    std::vector<std::shared_ptr<MenuModelPage>> childItems{};
    bool                                        isEditable{ false };
    bool                                        isSelectable{ false };
    SpecialKeyboardCallbacks                    specialKeyCallbacks;
    EventCallbacks                              eventCallbacks;
};
