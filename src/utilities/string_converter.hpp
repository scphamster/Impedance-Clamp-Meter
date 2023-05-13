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
        std::string string_buffer(Resolution + 1, 0);

        gcvtf(value, Resolution, string_buffer.data());

        return string_buffer;
    }
};
