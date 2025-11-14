#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "monitor.h"
#include "telegram_bot.h" // for sending messages

// External Wi-Fi event group from main
extern EventGroupHandle_t wifi_event_group;
extern const int WIFI_CONNECTED_BIT;

static const char *TAG = "MONITOR";

// Configuration (defaults)
#define SENSOR_ADC_CHANNEL    ADC_CHANNEL_6 // GPIO 34
#define DRY_THRESHOLD         2000

// Detection interval (minutes). Default will be set by macro in main; adjustable.
static int check_interval_minutes = 1;

static TaskHandle_t monitor_task_handle = NULL;
static bool running = false;
static int last_value = 0;

static void monitor_task(void *pvParameters)
{
    // Wait until Wi-Fi connected so that alerts can be sent
    ESP_LOGI(TAG, "Waiting for Wi-Fi...");
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    ESP_LOGI(TAG, "Wi-Fi ready, starting moisture detection.");

    // Initialize ADC
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = { .unit_id = ADC_UNIT_1 };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t cfg = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, SENSOR_ADC_CHANNEL, &cfg));

    bool alert_sent = false;

    while (running) {
        int sensor = 0;
        esp_err_t err = adc_oneshot_read(adc1_handle, SENSOR_ADC_CHANNEL, &sensor);
        if (err == ESP_OK) {
            last_value = sensor;
            ESP_LOGI(TAG, "Moisture reading: %d", sensor);

            if (sensor > DRY_THRESHOLD && !alert_sent) {
                char msg[80];
                snprintf(msg, sizeof(msg), "Your plant is thirsty! Level: %d", sensor);
                telegram_send_message(msg);
                alert_sent = true;
            } else if (sensor < DRY_THRESHOLD && alert_sent) {
                ESP_LOGI(TAG, "Plant appears watered, reset alert flag.");
                alert_sent = false;
            }
        } else {
            ESP_LOGE(TAG, "ADC read failed: %d", err);
        }

        // Sleep for configured interval (minutes)
        vTaskDelay(pdMS_TO_TICKS(check_interval_minutes * 60 * 1000));
    }

    // Clean up and exit
    ESP_LOGI(TAG, "Monitor task exiting.");
    vTaskDelete(NULL);
}

void monitor_start(void)
{
    if (running) return;
    running = true;
    xTaskCreate(&monitor_task, "monitor_task", 8 * 1024, NULL, 5, &monitor_task_handle);
}

void monitor_stop(void)
{
    if (!running) return;
    running = false;
    // Let the task self-delete; it will exit its loop once running=false
}

bool monitor_is_running(void)
{
    return running;
}

int monitor_get_last_value(void)
{
    return last_value;
}

void monitor_set_interval_minutes(int minutes)
{
    if (minutes <= 0) return;
    check_interval_minutes = minutes;
}
