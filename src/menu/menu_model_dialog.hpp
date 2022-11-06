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

    void SetOkCallback(ButtonCallback &&callback) noexcept { okCallback = std::move(callback); }
    void SetBackCallback(ButtonCallback &&callback) noexcept { backCallback = std::move(callback); }
    void SetMsg(MessageT &&new_msg) noexcept { message = std::move(new_msg); }
    void SetValue(std::shared_ptr<ValueT> new_value) noexcept
    {
        value    = std::move(new_value);
        hasValue = true;
    }
    void Show() noexcept { onShowCallback(); }
    void InvokeOkCallback() noexcept
    {
        if (std::shared_ptr<ValueT> my_value; okCallback and (my_value = value.lock())) {
            okCallback(my_value);
        }
        decisionMadeSemaphore->Give();
    }
    void InvokeBackCallback() noexcept
    {
        if (std::shared_ptr<ValueT> my_value; backCallback and (my_value = value.lock())) {
            backCallback(my_value);
        }
        decisionMadeSemaphore->Give();
    }
    void KeyboardHandler(Key key)
    {
        switch (key) {
        case Key::Enter: InvokeOkCallback(); break;
        case Key::Back: InvokeBackCallback(); break;
        }
    }
    void HasBeenDrawn() noexcept { redrawRequired = false; }
    void SetOnShowCallback(ShowCallback &&callback) noexcept { onShowCallback = std::move(callback); }
    void DeleteValue() noexcept { hasValue = false; }

    bool                                  HasValue() const noexcept { return hasValue; }
    bool                                  WaitForUserDecision() noexcept { return decisionMadeSemaphore->TakeWithTimeoutTicks(); }
    [[nodiscard]] MessageT                GetMessage() const noexcept { return message; }
    [[nodiscard]] bool                    RedrawRequired() const noexcept { return redrawRequired; }
    [[nodiscard]] bool                    IsShown() const noexcept { return isShown; }
    [[nodiscard]] std::shared_ptr<ValueT> GetValue() noexcept
    {
        if (std::shared_ptr<ValueT> val = value.lock())
            return val;

        else
            std::terminate();   // todo: implement better exception handling
    }
    [[nodiscard]] DialogType GetDialogType() const noexcept { return type; }

  private:
    MessageT              message{ "no message" };
    std::weak_ptr<ValueT> value;
    bool                  decision;

    std::shared_ptr<Semaphore> decisionMadeSemaphore;

    ButtonCallback okCallback;
    ButtonCallback backCallback;

    ShowCallback onShowCallback;

    DialogType type{ DialogType::Informational };

    // drawing section
    bool redrawRequired = true;
    bool isShown        = false;
    bool hasValue       = false;
};