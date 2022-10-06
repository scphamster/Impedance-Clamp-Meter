#pragma once
#include "pin.hpp"

template<typename IODriverT>
concept IODriver = requires (IODriverT driver) {
                       1 + 1;
                   };