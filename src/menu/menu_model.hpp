#pragma once
#include "compiler_compatibility_workaround.hpp"
#include <memory>

template<KeyboardC Keyboard>
class MenuModelPage;

template<KeyboardC Keyboard>
class MenuModel {
  public:
    using Item = MenuModelPage<Keyboard>;

    enum class KeyGroup {
        Transferable = 0,
        Persistent
    };

    MenuModel() = default;

    [[nodiscard]] std::shared_ptr<Item> GetTopLevelItem() const noexcept { return topLevelItem; }
    [[nodiscard]] std::shared_ptr<Item> GetCurrentPage() const noexcept { return currentItem; }

    void SetTopLevelItem(std::shared_ptr<Item> top_item) noexcept
    {
        topLevelItem = top_item;
        SetCurrentItem(topLevelItem);
    }
    void SetCurrentItem(std::shared_ptr<Item> new_current_item) noexcept
    {
        currentItem = new_current_item;
        currentItem->InvokeEventCallback(Item::Event::Entrance);
    }

  private:
    std::shared_ptr<Item> topLevelItem;
    std::shared_ptr<Item> currentItem;
};
