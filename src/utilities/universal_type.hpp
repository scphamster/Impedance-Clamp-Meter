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

// class _UniversalType : public std::variant<int, float, std::string> {
//   public:
//     using variant   = std::variant<int, float, std::string>;
//     UniversalType() = default;
//     UniversalType(const auto &other)
//       : variant::variant(other)
//     { }
//     UniversalType &operator=(const auto &rhs)
//     {
//         variant::operator=(rhs);
//         return *this;
//     }
//     UniversalType(auto &&other)
//       : variant::variant(std::move(other))
//     { }
//     UniversalType &operator=(auto &&rhs)
//     {
//         variant::operator=(std::move(rhs));
//         return *this;
//     }
// };