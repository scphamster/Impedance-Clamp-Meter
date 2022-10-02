#pragma once
#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#ifdef printf
#undef printf
#endif

#include <functional>
#include <vector>
#include <utility>
#include <algorithm>

template<typename KeyboardType>
concept Keyboard = requires { 1 + 1; };

class AbstractKeyboard {
  public:
};

class KeyboardMCP23016 : private AbstractKeyboard {
  public:
    enum class KeyIdent {
        Enter = 0,
        Down,
        Up,
        Left,
        Right,
        F1,
        F2,
        F3,
        F4
    };
    using Callback = std::function<void(KeyIdent)>;

    void SetCallbackForKeys(std::vector<KeyIdent> keys, Callback callback)
    {
        for (const auto &key : keys) {
            // search in callbacks if key is present, if not -> add ... else -> exchange --->>
            auto key_function_It =
              std::find_if(callbacks.begin(), callbacks.end(), [wanted_key = key](const auto &key_func) {
                  if (key_func.first == wanted_key)
                      return true;
                  else
                      return false;
              });

            if (key_function_It == callbacks.end()) {
                callbacks.push_back({ key, callback });
            }
            else {
                key_function_It->second = callback;
            }
        }

        std::sort(callbacks.begin(), callbacks.end(), [](const auto &key_func_left, const auto &key_func_right) {
            return key_func_left.first < key_func_right.first;
        });
    }

  private:
    std::vector<std::pair<KeyIdent, Callback>> callbacks;
};