#pragma once

#include "compiler_compatibility_workaround.hpp"
#include <cstdint>
#include <compiler.h>
#include <array>

template<typename ValueT, size_t BufferLen>
class DeviationCalculator {
  public:



  private:
    std::array<ValueT, BufferLen> buffer;

};