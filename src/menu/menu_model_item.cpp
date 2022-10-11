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
#include <utility>

#include "menu_model_item.hpp"

MenuModelPageItemData::NameT
MenuModelPageItemData::GetName() const noexcept
{
    return name;
}

UniversalType
MenuModelPageItemData::GetValue() const noexcept
{
    return value;
}





std::shared_ptr<MenuModelPageItemData>
MenuModelPageItem::GetData() const noexcept
{
    return data;
}

void
MenuModelPageItem::SetData(std::shared_ptr<MenuModelPageItemData> new_data) noexcept
{
    data = std::move(new_data);
}

MenuModelIndex
MenuModelPageItem::GetPosition() const noexcept
{
    return residesAtIndex;
}

bool
MenuModelPageItem::HasChild(MenuModelIndex at_position) const noexcept
{
    return (childItems.size() > 0) ? true : false;
}

std::shared_ptr<MenuModelPageItem>
MenuModelPageItem::GetChild(MenuModelIndex at_position) const noexcept
{
    if (at_position >= childItems.size())
        return *(childItems.end() - 1);

    return childItems.at(at_position);
}

void
MenuModelPageItem::InsertChild(std::shared_ptr<MenuModelPageItem> child, MenuModelIndex at_position) noexcept
{
    if (at_position < 0)
        return;

    if (childItems.size() <= at_position) {
        child->SetIndex(childItems.size());
        child->SetParent(this);
        childItems.emplace_back(std::move(child));
    }
    else {
        childItems.at(at_position) = std::move(child);
        child->SetParent(this);
    }
}

void
MenuModelPageItem::SetIndex(MenuModelIndex idx) noexcept
{
    residesAtIndex = (idx);
}

// void
// MenuModelPageItem::SetColumn(MenuModelIndex::Column new_column) noexcept
//{
//     residesAtIndex.SetColumn(new_column);
// }

MenuModelIndex
MenuModelPageItem::GetSelection() const noexcept
{
    return childSelectionIndex;
}

void
MenuModelPageItem::SetSelectionPosition(MenuModelIndex selected_item_position) noexcept
{
    childSelectionIndex = selected_item_position;
}

bool
MenuModelPageItem::SomeItemIsSelected() const noexcept
{
    return isSomeChildSelected;
}

void
MenuModelPageItem::SetEddittingCursorPosition(MenuModelPageItem::EditorCursorPosT new_position) noexcept
{
    edittingCursorPosition = new_position;
}

MenuModelPageItem::EditorCursorPosT
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
void
MenuModelPageItem::InsertChild(std::shared_ptr<MenuModelPageItem> child) noexcept
{
    InsertChild(child, child->GetPosition());
}

MenuModelPageItem *
MenuModelPageItem::GetParent() const noexcept
{
    return parent;
}

void
MenuModelPageItem::SetParent(MenuModelPageItem *new_parent) noexcept
{
    parent = new_parent;
}

std::vector<std::shared_ptr<MenuModelPageItem>>
MenuModelPageItem::GetChildren() const noexcept
{
    return childItems;
}

// MenuModelIndex::Column
// MenuModelIndex::GetColumn() const noexcept
//{
//     return position.first;
// }
//
// MenuModelIndex::Row
// MenuModelIndex::GetRow() const noexcept
//{
//     return position.second;
// }
//
// void
// MenuModelIndex::SetColumn(MenuModelIndex::Column new_column) noexcept
//{
//     position.first = new_column;
// }
//
// void
// MenuModelIndex::SetIndex(MenuModelIndex::Row new_row) noexcept
//{
//     position.second = new_row;
// }
