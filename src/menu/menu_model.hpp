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

//#include "menu_model_item.hpp"
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


    explicit MenuModel(std ::unique_ptr<Keyboard> &&new_keyboard, std::shared_ptr<Mutex> new_mutex)
      : mutex{ std::move(new_mutex) }
      , keyboard{ std::forward<decltype(new_keyboard)>(new_keyboard) }
    {
        auto system_keys_callbacks = ;
        keyboard->SetCallbacks(system_keys_callbacks, )
    }

    [[nodiscard]] std::shared_ptr<MenuModelPageItem> GetTopLevelItem() const noexcept;
    [[nodiscard]] std::shared_ptr<MenuModelPageItem> GetCurrentItem() const noexcept;

    void SetTopLevelItem(std::shared_ptr<Item> top_item) noexcept
    {
        topLevelItem = top_item;
        currentItem  = topLevelItem;

        auto callbacks = topLevelItem->GetKeyboardCallbacks();
        keyboard->SetCallbacks(callbacks);
        keyboard->ResetCallbacksAndSet(callbacks);
        keyboard->SetCallbacks(callbacks, usual_group);

    }

  private:
    std::shared_ptr<Mutex>             mutex;
    std::shared_ptr<MenuModelPageItem> topLevelItem;
    std::shared_ptr<MenuModelPageItem> currentItem;
    std::unique_ptr<Keyboard>          keyboard;
};

template<KeyboardC Keyboard>
void
MenuModel<Keyboard>::SetTopLevelItem(std::shared_ptr<MenuModelPageItem> top_item) noexcept
{
    topLevelItem = top_item;
    currentItem  = topLevelItem;
}

template<KeyboardC Keyboard>
std::shared_ptr<MenuModelPageItem>
MenuModel<Keyboard>::GetTopLevelItem() const noexcept
{
    return topLevelItem;
}

template<KeyboardC Keyboard>
std::shared_ptr<MenuModelPageItem>
MenuModel<Keyboard>::GetCurrentItem() const noexcept
{
    return currentItem;
}
