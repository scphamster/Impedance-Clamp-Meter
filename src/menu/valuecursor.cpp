//
// Created by scpha on 13/10/2022.
//

#include "valuecursor.hpp"
void
ValueCursor::SetValueEditCursorPos(int new_position, bool activate_edit_cursor/* = true */) noexcept
{
    valueEditCursorPos = new_position;
    valueEditCursorState = activate_edit_cursor;
}
void
ValueCursor::ActivateEditCursor(bool if_activate) noexcept
{
    valueEditCursorState = if_activate;
}
int
ValueCursor::GetEditCursorPos() const noexcept
{
    return valueEditCursorPos;
}
bool
ValueCursor::IsEditCursorIsActive() const noexcept
{
    return valueEditCursorState;
}