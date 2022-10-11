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

#include <variant>
#include <string>

using UniversalType = std::variant<int, float, std::string>;