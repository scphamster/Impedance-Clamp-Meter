#pragma once

#include "misc/compiler_compatibility_workaround.hpp"
#include <string>
#include <array>

extern "C" char *gcvtf(float, int, char *);

class StringConverter {
  public:
    using String = std::string;

    template<size_t Resolution>
    String static ToString(float value) noexcept
    {
        std::array<char, Resolution + 6> string_buffer;

        gcvtf(value, Resolution, string_buffer.data());

        return string_buffer.data();
    }
};
