#pragma once
#include <cstdint>
using TickType_t = std::uint32_t;
using BaseType_t = int;
constexpr int pdPASS = 1, pdTRUE = 1;
constexpr TickType_t portMAX_DELAY = UINT32_MAX;
#define pdMS_TO_TICKS(value) (value)
