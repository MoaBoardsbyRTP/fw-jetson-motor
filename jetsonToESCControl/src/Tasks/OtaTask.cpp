/**
 * @file OtaTask.cpp
 * @brief FreeRTOS task for ArduinoOTA handling
 * @author Oscar Martinez
 * @date 2026-02-27
 */

#include "Tasks.h"
#include "MoaMainUnit.h"
#include "esp_log.h"

static const char* TAG = "OtaTask";

/**
 * @brief OTA polling period (ms)
 * Needs to be responsive for OTA discovery and transfer.
 */
#define TASK_OTA_PERIOD_MS 50

void OtaTask(void* pvParameters) {
    MoaMainUnit* unit = static_cast<MoaMainUnit*>(pvParameters);

    ESP_LOGI(TAG, "OtaTask started");
    // OTA lifecycle controlled by ConfigState - no begin() here

    for (;;) {
        unit->getOTAManager().handle();
        vTaskDelay(pdMS_TO_TICKS(TASK_OTA_PERIOD_MS));
    }
}
