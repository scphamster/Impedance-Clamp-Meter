#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"

void QueueFullHook(TaskHandle_t task, char *task_name);

#ifdef __cplusplus
}
#endif