/**
 * @file CliTask.cpp
 * @brief FreeRTOS task for UART CLI polling
 * @author Oscar Martinez
 * @date 2026-02-15
 */

#include "Tasks.h"
#include "MoaMainUnit.h"
#include "esp_log.h"

static const char* TAG = "CliTask";

/**
 * @brief CLI polling period (ms)
 * Low frequency is fine — human typing speed is the bottleneck.
 */
#define TASK_CLI_PERIOD_MS 50

void CliTask(void* pvParameters) {
    MoaMainUnit* unit = static_cast<MoaMainUnit*>(pvParameters);

    ESP_LOGI(TAG, "CliTask started");

    unit->getUartCli().begin();

    for (;;) {
        unit->getUartCli().poll();
        vTaskDelay(pdMS_TO_TICKS(TASK_CLI_PERIOD_MS));
    }
}
