//
// File Path: components/sensors/sensors.c
// Brief:     Sensing logic for ModMesh Actuator Node.
// Author:    M. YOUCEF Yazid (yazid.youcef@gmail.com)
// Version:   0.8.1 (Actuator Edition)
// CreateDate: 2026-04-26
// UpdateDate: 2026-05-11
//

#include "sensors.h"
#include "shared_config.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <stdio.h>
#include <string.h>

#if DEVICE_ROLE == ROLE_SENSOR || DEVICE_ROLE == ROLE_BOTH

static const char *TAG = "sensors";

void sensors_init(void)
{
    ESP_LOGI(TAG, "Initializing local inputs (if any)...");
    // Default implementation has no specific sensors for pure Actuator role.
}

void sensors_force_initial_state(void)
{
    ESP_LOGW(TAG, "Actuator-side sensor states reset.");
}

void sensors_read(char *buffer, size_t max_len)
{
    if (!buffer || max_len == 0) return;
    // Broadcast a simple heartbeat/status string
    snprintf(buffer, max_len, "[%s]STATUS:READY", DATA_KEYWORDS);
}

#endif // DEVICE_ROLE check
