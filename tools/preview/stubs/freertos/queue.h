#pragma once
#include "FreeRTOS.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sim_queue *QueueHandle_t;
QueueHandle_t xQueueCreate(size_t length, size_t item_size);
BaseType_t xQueueSend(QueueHandle_t q, const void *item, TickType_t wait);
BaseType_t xQueueOverwrite(QueueHandle_t q, const void *item);
BaseType_t xQueueReceive(QueueHandle_t q, void *item, TickType_t wait);
#ifdef __cplusplus
}
#endif
