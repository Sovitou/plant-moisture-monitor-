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

// Include the modern ADC driver for reading the sensor
#include "esp_adc/adc_oneshot.h"

// --- START: YOUR CONFIGURATION ---

// -- Wi-Fi Credentials --
#define WIFI_SSID       "MSI"
#define WIFI_PASSWORD   "12345678"

// -- Telegram Bot Details --
#define BOT_TOKEN       "8564970952:AAFwwmQco2ZvV863oQATC2vyjYvSwX1k4lQ"
#define CHAT_ID         "-5029281515"

// -- Sensor and Logic Settings --
#define SENSOR_ADC_CHANNEL    ADC_CHANNEL_6 // GPIO 34 is ADC1_CHANNEL_6
#define DRY_THRESHOLD         2000          // !!! IMPORTANT: Calibrate this value for your plant!
#define CHECK_INTERVAL_MINUTES 30           // How often to check the moisture (in minutes)

// --- END: YOUR CONFIGURATION ---


static const char *TAG = "PLANT_MONITOR";

// Event group to signal when the Wi-Fi connection is established
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

// --- Function Prototypes ---
static void send_telegram_alert(const char *message);


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

// Handler for HTTP client events (can be left minimal)
esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    return ESP_OK;
}

// Sends a message to your Telegram chat via your bot
static void send_telegram_alert(const char *message) {
    char url[256];
    char encoded_message[128] = {0};

    // Manually encode spaces in the message for the URL
    int i = 0, j = 0;
    while (message[i] != '\0' && j < sizeof(encoded_message) - 4) {
        if (message[i] == ' ') {
            encoded_message[j++] = '%';
            encoded_message[j++] = '2';
            encoded_message[j++] = '0';
        } else {
            encoded_message[j++] = message[i];
        }
        i++;
    }
    
    snprintf(url, sizeof(url), "https://api.telegram.org/bot%s/sendMessage?chat_id=%s&text=%s",
             BOT_TOKEN, CHAT_ID, encoded_message);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Telegram message sent successfully: %s", message);
    } else {
        ESP_LOGE(TAG, "Telegram message failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}


// =========================================================================
// Main Application Task
// =========================================================================

// The main task that reads the sensor and sends alerts
static void plant_monitor_task(void *pvParameters)
{
    // -- Step 1: Wait until Wi-Fi is connected --
    ESP_LOGI(TAG, "Plant monitor task started. Waiting for Wi-Fi connection...");
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    ESP_LOGI(TAG, "Wi-Fi signal received. Starting monitor.");

    // Send a startup message to confirm everything is working
    send_telegram_alert("Plant monitor is online!");

    // -- Step 2: Initialize the ADC to read the sensor --
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = { .unit_id = ADC_UNIT_1 };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12, // 12-bit resolution (0-4095)
        .atten = ADC_ATTEN_DB_12,    // Use full voltage range (up to 3.3V)
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, SENSOR_ADC_CHANNEL, &config));

    // Flag to prevent sending multiple "thirsty" alerts
    bool alert_has_been_sent = false;

    // -- Step 3: Start the infinite monitoring loop --
    while(1) {
        int sensor_value = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, SENSOR_ADC_CHANNEL, &sensor_value));
        ESP_LOGI(TAG, "Moisture Reading: %d", sensor_value);

        // Logic to check if the plant is dry
        if (sensor_value > DRY_THRESHOLD && !alert_has_been_sent) {
            ESP_LOGI(TAG, "Soil is DRY. Sending Telegram alert.");
            // Format a message that includes the sensor value for debugging
            char message[64];
            sprintf(message, "Your plant is thirsty! Level: %d", sensor_value);
            send_telegram_alert(message);
            alert_has_been_sent = true; // Set the flag so we don't send again
        }
        // Logic to reset the flag once the plant has been watered
        else if (sensor_value < DRY_THRESHOLD && alert_has_been_sent) {
            ESP_LOGI(TAG, "Plant has been watered. Resetting alert flag.");
            alert_has_been_sent = false; // Reset the flag
        }

        // Wait for the specified interval before checking again
        ESP_LOGI(TAG, "Sleeping for %d minutes...", CHECK_INTERVAL_MINUTES);
        vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MINUTES * 60 * 1000));
    }
}


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
    
    // Create the main application task
    xTaskCreate(&plant_monitor_task, "plant_monitor_task", 8192, NULL, 5, NULL);
}
