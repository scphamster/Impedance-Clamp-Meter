#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#ifdef printf
#undef printf
#endif

#include <string>

#include "menu_model_item.hpp"

std::string
MenuModelPageItemData::GetName() const noexcept
{
    return std::string();
}

std::shared_ptr<MenuModelPageItemData>
MenuModelPageItem::GetData() const noexcept
{
    return data;
}

void
MenuModelPageItem::SetPosition(const MenuModelIndex &new_position) noexcept
{
    residesAtIndex = new_position;
}

void
MenuModelPageItem::SetData(std::shared_ptr<MenuModelPageItemData> new_data) noexcept
{
    data = new_data;
}

MenuModelIndex
MenuModelPageItem::GetPosition() const noexcept
{
    return residesAtIndex;
}

bool
MenuModelPageItem::HasChild(const MenuModelIndex &at_position) const noexcept
{
    return false;
}

std::shared_ptr<MenuModelPageItem>
MenuModelPageItem::GetChild(const MenuModelIndex &at_position) const noexcept
{
    // todo: implement
}

void
MenuModelPageItem::InsertChild(std::shared_ptr<MenuModelPageItem> child, const MenuModelIndex &at_position)
{
    // todo: implement
}

void
MenuModelPageItem::SetRow(MenuModelIndex::Row new_row) noexcept
{
    residesAtIndex.SetRow(new_row);
}

void
MenuModelPageItem::SetColumn(MenuModelIndex::Column new_column) noexcept
{
    residesAtIndex.SetColumn(new_column);
}

MenuModelIndex
MenuModelPageItem::GetSelection() const noexcept
{
    return childSelectionIndex;
}

void
MenuModelPageItem::SetSelectionPosition(const MenuModelIndex &selected_item_position) noexcept
{
    childSelectionIndex = selected_item_position;
}

bool
MenuModelPageItem::SomeItemIsSelected() const noexcept
{
    return isSomeChildSelected;
}

void
MenuModelPageItem::SetEddittingCursorPosition(MenuModelPageItem::EdittingCursorT new_position) noexcept
{
    edittingCursorPosition = new_position;
}

MenuModelPageItem::EdittingCursorT
MenuModelPageItem::GetEdittingCursorPosition() const noexcept
{
    return edittingCursorPosition;
}

bool
MenuModelPageItem::EdittingCursorIsActive() const noexcept
{
    return editCursorActive;
}

void
MenuModelPageItem::ActivateEdittingCursor(bool if_activate) noexcept
{
    editCursorActive = if_activate;
}

MenuModelIndex::Column
MenuModelIndex::GetColumn() const noexcept
{
    return position.first;
}

MenuModelIndex::Row
MenuModelIndex::GetRow() const noexcept
{
    return position.second;
}

void
MenuModelIndex::SetColumn(MenuModelIndex::Column new_column) noexcept
{
    position.first = new_column;
}

void
MenuModelIndex::SetRow(MenuModelIndex::Row new_row) noexcept
{
    position.second = new_row;
}
