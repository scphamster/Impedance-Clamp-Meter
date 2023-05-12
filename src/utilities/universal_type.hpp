#pragma once
#include "misc/compiler_compatibility_workaround.hpp"
#include <utility>
#include <string>
#include <variant>
#include <mutex>

#include "semaphore.hpp"

using UniversalType = std::variant<int, float, std::string>;

class UniversalSafeType {
  public:
    using IntegerType = int;
    using FloatType   = float;
    using StringType  = std::string;
    using ValueType   = UniversalType;

    UniversalSafeType() = default;

    explicit UniversalSafeType(ValueType &&new_value)
      : value{ std::move(new_value) }
    { }

    explicit UniversalSafeType(UniversalSafeType &other)
      : value{ other.value }
    { }

    UniversalSafeType &operator=(const UniversalSafeType &other)
    {
        value = other.value;
        return *this;
    }

    UniversalSafeType &operator=(ValueType const &new_value)
    {
        value = new_value;
        return *this;
    }

    void SetValue(ValueType new_value) noexcept
    {
        std::lock_guard<Mutex>{ mutex };
        value = std::move(new_value);
    }
    [[nodiscard]] ValueType GetValue() noexcept
    {
        std::lock_guard<Mutex>{ mutex };
        return value;
    }

    bool operator==(const auto &rhs) const
    {
        std::lock_guard<Mutex>{ mutex };

        if (value == rhs.value)
            return true;
        else
            return false;
    }

  private:
    ValueType     value;
    mutable Mutex mutex;
};
