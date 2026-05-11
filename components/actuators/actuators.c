//
// File Path: components/actuators/actuators.c
// Brief:     Actuator management for ModMesh Actuator Node.
// Author:    M. YOUCEF Yazid (yazid.youcef@gmail.com)
// Version:   0.8.1 (Actuator Edition)
// CreateDate: 2026-04-26
// UpdateDate: 2026-05-11
//

#include "actuators.h"
#include "shared_config.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#if DEVICE_ROLE == ROLE_ACTUATOR || DEVICE_ROLE == ROLE_BOTH

static const char *TAG = "actuators";

#define MAX_PAYLOAD_LEN 128
typedef struct {
    char payload[MAX_PAYLOAD_LEN];
} actuator_cmd_t;

static QueueHandle_t s_actuator_queue = NULL;

static void process_command(const char *payload);

static void actuator_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Actuator task started (Hardware Control Monitor)");
    actuator_cmd_t cmd;

    while (1) {
        if (xQueueReceive(s_actuator_queue, &cmd, portMAX_DELAY) == pdTRUE) {
            process_command(cmd.payload);
        }
    }
}

void actuators_init(void)
{
    ESP_LOGI(TAG, "Initializing hardware actuator (GPIO %d)...", ACTUATOR_OUTPUT_GPIO);

    gpio_set_direction(ACTUATOR_OUTPUT_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(ACTUATOR_OUTPUT_GPIO, 0); // Default OFF

    s_actuator_queue = xQueueCreate(10, sizeof(actuator_cmd_t));
    if (s_actuator_queue != NULL) {
        xTaskCreate(actuator_task, "actuator_task", 4096, NULL, 7, NULL);
    } else {
        ESP_LOGE(TAG, "Failed to create actuator queue!");
    }
}

void actuators_execute(const char *payload)
{
    if (!payload || !s_actuator_queue) return;

    actuator_cmd_t cmd;
    strncpy(cmd.payload, payload, MAX_PAYLOAD_LEN - 1);
    cmd.payload[MAX_PAYLOAD_LEN - 1] = '\0';

    if (xQueueSend(s_actuator_queue, &cmd, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Actuator queue full!");
    }
}

static void process_command(const char *payload)
{
    // Filter messages based on Keywords (Pub/Sub routing)
    bool keyword_match = false;
    
    const char *app_payload = strchr(payload, ']');
    if (app_payload) {
        app_payload++; 
        const char *keyword_start = strchr(app_payload, '[');
        if (keyword_start) {
            const char *keyword_end = strchr(keyword_start, ']');
            if (keyword_end) {
                size_t list_len = keyword_end - keyword_start - 1;
                char target_list[128];
                if (list_len < sizeof(target_list)) {
                    strncpy(target_list, keyword_start + 1, list_len);
                    target_list[list_len] = '\0';
                    
                    char *payload_token = strtok(target_list, ",");
                    while (payload_token != NULL) {
                        // Check for global broadcast or specific node label
                        if (strcmp(payload_token, "ALL") == 0 || strcmp(payload_token, NODE_LABEL) == 0) {
                            keyword_match = true;
                            break;
                        }
                        
                        // Check if payload_token exists in our ACTUATOR_KEYWORDS
                        char my_keywords[128];
                        strncpy(my_keywords, ACTUATOR_KEYWORDS, sizeof(my_keywords));
                        char *my_token = strtok(my_keywords, ",");
                        while (my_token != NULL) {
                            if (strcmp(payload_token, my_token) == 0) {
                                keyword_match = true;
                                break;
                            }
                            my_token = strtok(NULL, ",");
                        }
                        
                        if (keyword_match) break;
                        payload_token = strtok(NULL, ",");
                    }
                }
            }
        }
    }
    
    if (!keyword_match) return;

    // --- EMERGENCY NETWORK RESET ---
    if (strstr(payload, "CMD:NETWORK_RESET") != NULL) {
        ESP_LOGW(TAG, "⚡ NETWORK EMERGENCY RESET RECEIVED! Killing outputs.");
        gpio_set_level(ACTUATOR_OUTPUT_GPIO, 0);
        return; 
    }

    // --- Hardware Control Logic ---
    if (strstr(payload, "STATE:1") != NULL || strstr(payload, "CMD:LED_ON") != NULL) {
        ESP_LOGW(TAG, "⚡ ACTUATOR TRIGGERED: Setting GPIO %d to HIGH", ACTUATOR_OUTPUT_GPIO);
        gpio_set_level(ACTUATOR_OUTPUT_GPIO, 1);
    } 
    else if (strstr(payload, "STATE:0") != NULL || strstr(payload, "CMD:LED_OFF") != NULL) {
        ESP_LOGI(TAG, "⚡ ACTUATOR TRIGGERED: Setting GPIO %d to LOW", ACTUATOR_OUTPUT_GPIO);
        gpio_set_level(ACTUATOR_OUTPUT_GPIO, 0);
    }
}

#endif // DEVICE_ROLE check
