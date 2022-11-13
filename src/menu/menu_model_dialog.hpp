#pragma once
#include "compiler_compatibility_workaround.hpp"

#include <memory>
#include <string>
#include <functional>

#include "universal_type.hpp"
#include "semaphore/semaphore.hpp"

class MenuModelDialog {
  public:
    using MessageT       = std::string;
    using PositionT      = int;
    using ValueT         = UniversalSafeType;
    using ButtonCallback = std::function<void(std::shared_ptr<UniversalSafeType>)>;
    using ShowCallback   = std::function<void()>;

    enum class Button {
        OK,
        Cancel
    };

    enum class Key {
        Left,
        Right,
        Enter,
        Back
    };

    enum class DialogType {
        Informational,
        InputBox
    };

    explicit MenuModelDialog()
      : decisionMadeSemaphore{ std::make_shared<Semaphore>() }
    { }

    void Show() noexcept { onShowCallback(); }
    void InvokeOkCallback() noexcept
    {
        //        if (std::shared_ptr<ValueT> my_value; okCallback and (my_value = value.lock())) {
        //            okCallback(my_value);
        //        }
        //        decisionMadeSemaphore->Give();
    }
    void InvokeBackCallback() noexcept
    {
        //        if (std::shared_ptr<ValueT> my_value; backCallback and (my_value = value.lock())) {
        //            backCallback(my_value);
        //        }
        //        decisionMadeSemaphore->Give();
    }
    void KeyboardHandler(Key key)
    {
        SetIsShown(false);

        switch (key) {
        case Key::Enter: ButtonPressed(Button::OK); break;
        case Key::Back: ButtonPressed(Button::Cancel); break;
        }
    }

    void SetOkCallback(ButtonCallback &&callback) noexcept { okCallback = std::move(callback); }
    void SetBackCallback(ButtonCallback &&callback) noexcept { backCallback = std::move(callback); }
    void SetMsg(MessageT &&new_msg) noexcept { message = std::move(new_msg); }
    void ShowMsg(MessageT &&new_msg) noexcept
    {
        if (message == new_msg)
            return;

        SetMsg(std::forward<MessageT>(new_msg));
        Show();
    }
    void SetValue(std::shared_ptr<ValueT> new_value) noexcept
    {
        value    = std::move(new_value);
        hasValue = true;
    }
    void SetIsShown(bool if_shown) noexcept { isShown = if_shown; }

    void SetHasBeenDrawnFlag(bool if_has_been_drawn = true) noexcept { hasBeenDrawn = if_has_been_drawn; }
    void SetOnShowCallback(ShowCallback &&callback) noexcept { onShowCallback = std::move(callback); }
    void SetHasValue(bool if_has_value) noexcept { hasValue = if_has_value; }
    void SetType(DialogType new_type) noexcept { type = new_type; }

    bool ValueNeedsToBeRedrawn() noexcept
    {
        if (auto current_value = value.lock()) {
            if (*current_value == drawnValue)
                return false;
            else
                return true;
        }
        else
            return false;
    }
    bool HasValue() const noexcept { return hasValue; }
    void WaitForUserReaction() noexcept
    {
        isWaitingForUserInput = true;
        decisionMadeSemaphore->TakeBlockInfinitely();
        isWaitingForUserInput = false;
    }
    [[nodiscard]] MessageT                GetMessage() const noexcept { return message; }
    [[nodiscard]] bool                    HasBeenDrawn() const noexcept { return hasBeenDrawn; }
    [[nodiscard]] bool                    IsShown() const noexcept { return isShown; }
    [[nodiscard]] std::shared_ptr<ValueT> GetValue() noexcept
    {
        if (std::shared_ptr<ValueT> val = value.lock())
            return val;
        else
            std::terminate();   // todo: implement better exception handling
    }
    [[nodiscard]] DialogType GetDialogType() const noexcept { return type; }
    [[nodiscard]] bool       IsWaitingForUserInput() const noexcept { return isWaitingForUserInput; }

  protected:
    // slots
    void ButtonPressed(Button button)
    {
        if (button == Button::OK) {
            userDecision = true;
        }
        else if (button == Button::Cancel) {
            userDecision = false;
        }

        DecisionHasBeenTaken();
    }
    void DecisionHasBeenTaken() noexcept { decisionMadeSemaphore->Give(); }

  private:
    MessageT              message{ "no message" };
    std::weak_ptr<ValueT> value;
    ValueT                drawnValue;
    bool                  userDecision;

    std::shared_ptr<Semaphore> decisionMadeSemaphore;

    ButtonCallback okCallback;
    ButtonCallback backCallback;

    ShowCallback onShowCallback;

    DialogType type{ DialogType::Informational };

    // drawing section
    bool hasBeenDrawn{ true };
    bool isShown{ false };
    bool hasValue{ false };
    bool isWaitingForUserInput{ false };
};