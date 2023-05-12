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

    [[nodiscard]] std::shared_ptr<Item> GetTopLevelItem() const noexcept { return rootPage; }
    [[nodiscard]] std::shared_ptr<Item> GetCurrentPage() const noexcept { return currentPage; }

    void SetRootPage(std::shared_ptr<Item> root_page) noexcept
    {
        rootPage = root_page;
        SetCurrentPage(rootPage);
    }
    void SetCurrentPage(std::shared_ptr<Item> page) noexcept
    {
        currentPage = page;
        currentPage->InvokeEventCallback(Item::Event::Entrance);
    }

  private:
    std::shared_ptr<Item> rootPage;
    std::shared_ptr<Item> currentPage;
};
