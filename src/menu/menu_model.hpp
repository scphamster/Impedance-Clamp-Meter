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
#include <utility>

// #include "menu_model_item.hpp"
#include "semaphore.hpp"

template<KeyboardC Keyboard>
class MenuModelPageItem;

template<KeyboardC Keyboard>
class MenuModel {
  public:
    using Item = MenuModelPageItem<Keyboard>;

    enum class KeyGroup {
        Transferable = 0,
        Persistent
    };

    explicit MenuModel(/*std ::unique_ptr<Keyboard> &&new_keyboard, */std::shared_ptr<Mutex> new_mutex)
      : mutex{ std::move(new_mutex) }
    {
//        keyboard->SetButtonEventCallback(1, Keyboard::ButtonEvent::Push, [this](){currentItem->SetName("DUPA");});
    }

    [[nodiscard]] std::shared_ptr<Item> GetTopLevelItem() const noexcept { return topLevelItem; }
    [[nodiscard]] std::shared_ptr<Item> GetCurrentItem() const noexcept { return currentItem; }


    void SetTopLevelItem(std::shared_ptr<Item> top_item) noexcept
    {
        topLevelItem = top_item;
        SetCurrentItem(topLevelItem);
    }
    void SetCurrentItem(std::shared_ptr<Item> new_current_item) noexcept {
        currentItem = new_current_item;
        currentItem->InvokeEventCallback(Item::Event::Entrance);
    }


  private:
    std::shared_ptr<Mutex>    mutex;
    std::shared_ptr<Item>     topLevelItem;
    std::shared_ptr<Item>     currentItem;
//    std::unique_ptr<Keyboard> keyboard;
};
