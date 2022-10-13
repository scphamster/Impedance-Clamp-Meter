
#include "menu_model_drawer.hpp"
#include "valuecursor.hpp"

#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#ifdef printf
#undef printf
#endif

void
ItemCursor::SetItemNum(int new_item_num, bool activate_edit_cursor/* = true */) noexcept
{
    itemNum = new_item_num;
    itemCursorState = activate_edit_cursor;
}

void
ItemCursor::ActivateItemCursor(bool if_activate) noexcept
{
    itemCursorState = if_activate;
}
bool
ItemCursor::IsItemCursorIsActive() const noexcept
{
    return itemCursorState;
}
int
ItemCursor::GetItemCursorPos() const noexcept
{
    return itemNum;
}
int
