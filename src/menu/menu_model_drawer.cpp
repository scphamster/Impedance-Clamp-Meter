
#include "menu_model_drawer.hpp"

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
ItemCursor::Set(int new_item_num, bool activate_edit_cursor/* = true */) noexcept
{
    itemNum = new_item_num;
    itemCursorState = activate_edit_cursor;
}

void
ItemCursor::Activate(bool if_activate)  noexcept
{
    itemCursorState = if_activate;
}
bool
ItemCursor::IsActive() const noexcept
{
    return itemCursorState;
}
int
ItemCursor::GetPos() const noexcept
{
    return itemNum;
}
