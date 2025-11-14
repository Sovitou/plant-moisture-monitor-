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
#include "esp_http_server.h"

// Include the modern ADC driver for reading the sensor
#include "esp_adc/adc_oneshot.h"

// --- START: YOUR CONFIGURATION ---

// -- Wi-Fi Credentials --
#define WIFI_SSID       "Galaxy Tab A9+ 5G 1159"
#define WIFI_PASSWORD   "vitou2304"

// -- Telegram Bot Details --
const char *BOT_TOKEN = "8564970952:AAFwwmQco2ZvV863oQATC2vyjYvSwX1k4lQ";
const char *CHAT_ID = "-5029281515";

// -- Sensor and Logic Settings --
#define CHECK_INTERVAL_MINUTES 1 // Check every 1 minute

// --- END: YOUR CONFIGURATION ---


static const char *TAG = "PLANT_MONITOR";

// Event group to signal when the Wi-Fi connection is established
EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

// --- Function Prototypes ---
// forward declarations for modules
void telegram_send_message(const char *message);
httpd_handle_t start_http_server(void);
void telegram_start(void);
void telegram_stop(void);
void monitor_start(void);
void monitor_stop(void);
void monitor_set_interval_minutes(int minutes);


// =========================================================================
// Wi-Fi Connection Logic
// =========================================================================

// Event handler for Wi-Fi events. Its only job is to signal connection success.
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected from Wi-Fi, retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Wi-Fi Connected, got IP address.");
        // Signal that the connection is successful by setting the bit
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Initializes and starts the Wi-Fi connection process
void wifi_init_sta(void) {
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = { .sta = { .ssid = WIFI_SSID, .password = WIFI_PASSWORD, }, };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Wi-Fi initialization complete.");
}


// =========================================================================
// Telegram Communication
// =========================================================================

// Note: telegram send is implemented in telegram_bot.c (telegram_send_message)


// =========================================================================
// Main Application Task
// =========================================================================

// The main task that reads the sensor and sends alerts
// The actual monitoring logic is implemented in monitor.c


// =========================================================================
// Main Function
// =========================================================================

void app_main(void)
{
    // Initialize NVS Flash (required for Wi-Fi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Start Wi-Fi
    wifi_init_sta();
    
    // Wait for Wi-Fi to connect before starting services
    ESP_LOGI(TAG, "Waiting for Wi-Fi connection...");
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    ESP_LOGI(TAG, "Wi-Fi connected! Starting services...");

    // Start the monitoring subsystem (will wait for Wi-Fi internally)
    monitor_set_interval_minutes(CHECK_INTERVAL_MINUTES);
    monitor_start();
    ESP_LOGI(TAG, "Monitor started");

    // Start HTTP server to expose /status, /start, /stop
    httpd_handle_t server = start_http_server();
    if (server) {
        ESP_LOGI(TAG, "HTTP server initialized successfully");
    } else {
        ESP_LOGE(TAG, "HTTP server failed to start!");
    }

    // Start Telegram polling bot
    telegram_start();
    ESP_LOGI(TAG, "Telegram bot started");
    
    ESP_LOGI(TAG, "All services started. System ready.");
}
