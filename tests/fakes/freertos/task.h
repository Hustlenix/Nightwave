#pragma once
#include "freertos/FreeRTOS.h"
struct FakeTask;
using TaskHandle_t = FakeTask*;
BaseType_t xTaskCreate(void (*entry)(void*), const char*, std::uint32_t,
                      void*, unsigned, TaskHandle_t*);
void vTaskDelete(TaskHandle_t);
void vTaskDelay(TickType_t);
void xTaskNotifyGive(TaskHandle_t);
std::uint32_t ulTaskNotifyTake(int, TickType_t);
unsigned uxTaskGetStackHighWaterMark(TaskHandle_t);
