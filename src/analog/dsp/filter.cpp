#include "filter.hpp"
#include "arm_math.h"

AbstractFilter::~AbstractFilter() = default;

extern "C" void SuperFilterTask(void *param) {
    auto filter_instance = static_cast<SuperFilter<float, 100, 1> *>(param);

    filter_instance->InputQueueTask();
}
