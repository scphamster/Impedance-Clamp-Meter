//
// Created by scpha on 13/10/2022.
//

#ifndef CLAMPMETER_VALUECURSOR_HPP
#define CLAMPMETER_VALUECURSOR_HPP

class ValueCursor {
  public:
    void               SetValueEditCursorPos(int new_position, bool activate_edit_cursor = true) noexcept;
    void               ActivateEditCursor(bool if_activate) noexcept;
    [[nodiscard]] int  GetEditCursorPos() const noexcept;
    [[nodiscard]] bool IsEditCursorIsActive() const noexcept;

  private:
    int  valueEditCursorPos{ 0 };
    bool valueEditCursorState{ false };
};

#endif   // CLAMPMETER_VALUECURSOR_HPP
