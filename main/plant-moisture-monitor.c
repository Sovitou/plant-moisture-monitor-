/*
 * ============================================================================
 * PLANT MOISTURE MONITOR - Simple Version
 * ============================================================================
 * 
 * What this program does:
 * 1. Connects to Wi-Fi
 * 2. Reads soil moisture sensor every minute
 * 3. Sends Telegram alert when plant needs water
 * 
 * Hardware:
 * - ESP32 board
 * - Moisture sensor connected to GPIO 34
 * 
 * How to configure:
 * - Edit the Wi-Fi settings below
 * - Edit the Telegram bot token and chat ID
 * - Adjust the dry threshold if needed
 * 
 * ============================================================================
 */

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_adc/adc_oneshot.h"

// =============================================================================
// CONFIGURATION - Edit these values for your setup
// =============================================================================

// Wi-Fi Credentials
#define WIFI_SSID       "Galaxy Tab A9+ 5G 1159"
#define WIFI_PASSWORD   "vitou2304"

// Telegram Bot Details
#define BOT_TOKEN       "8564970952:AAFwwmQco2ZvV863oQATC2vyjYvSwX1k4lQ"
#define CHAT_ID         "-5029281515"

// Sensor Settings
#define SENSOR_ADC_CHANNEL    ADC_CHANNEL_6  // GPIO 34
#define DRY_THRESHOLD         2000           // ADC value above this means plant needs water
#define CHECK_INTERVAL_MINUTES 1             // How often to check moisture level

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================

static const char *TAG = "PLANT_MONITOR";
EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

// =============================================================================
// WI-FI CONNECTION
// =============================================================================

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected from Wi-Fi, retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Wi-Fi Connected!");
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init(void) {
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Connecting to Wi-Fi...");
}

// =============================================================================
// TELEGRAM ALERT
// =============================================================================

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    return ESP_OK;
}

void send_telegram_alert(const char *message) {
    if (!message) return;
    
    // Build Telegram API URL (without the message in URL)
    char url[256];
    snprintf(url, sizeof(url), 
             "https://api.telegram.org/bot%s/sendMessage", 
             BOT_TOKEN);

    // Build POST data with chat_id and text
    char post_data[512];
    snprintf(post_data, sizeof(post_data), 
             "chat_id=%s&text=%s", 
             CHAT_ID, message);

    // Send HTTP POST request
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 15000,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return;
    }
    
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Alert sent to Telegram (status: %d)", status);
    } else {
        ESP_LOGE(TAG, "Failed to send alert: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
}

// =============================================================================
// MOISTURE MONITORING
// =============================================================================

void monitor_task(void *pvParameters) {
    // Wait for Wi-Fi connection
    ESP_LOGI(TAG, "Waiting for Wi-Fi...");
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    ESP_LOGI(TAG, "Starting moisture monitoring...");

    // Initialize ADC for reading moisture sensor
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    adc_oneshot_chan_cfg_t channel_config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, SENSOR_ADC_CHANNEL, &channel_config));

    // Main monitoring loop
    while (1) {
        int sensor_value = 0;
        esp_err_t err = adc_oneshot_read(adc1_handle, SENSOR_ADC_CHANNEL, &sensor_value);
        
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Moisture level: %d", sensor_value);

            // If soil is dry, send alert every time
            if (sensor_value > DRY_THRESHOLD) {
                char alert_message[80];
                snprintf(alert_message, sizeof(alert_message), 
                         "🌱 Your plant needs water! Moisture level: %d", sensor_value);
                send_telegram_alert(alert_message);
            } else {
                ESP_LOGI(TAG, "Plant moisture is OK");
            }
        } else {
            ESP_LOGE(TAG, "Failed to read sensor: %d", err);
        }

        // Wait before next check
        vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MINUTES * 60 * 1000));
    }
}

// =============================================================================
// MAIN FUNCTION
// =============================================================================

void app_main(void) {
    ESP_LOGI(TAG, "Plant Moisture Monitor Starting...");

    // Initialize NVS (required for Wi-Fi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Connect to Wi-Fi
    wifi_init();

    // Start monitoring task
    xTaskCreate(&monitor_task, "monitor", 8192, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "System ready!");
}
