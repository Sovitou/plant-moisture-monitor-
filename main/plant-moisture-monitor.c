#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h" // For signaling between tasks
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"

// --- START: Your Details ---
#define WIFI_SSID       "MSI"
#define WIFI_PASSWORD   "12345678"
#define BOT_TOKEN       "8564970952:AAFwwmQco2ZvV863oQATC2vyjYvSwX1k4lQ"
#define CHAT_ID         "-5029281515"
// --- END: Your Details ---

static const char *TAG = "WIFI_TEST";

// --- FreeRTOS event group to signal when we are connected ---
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

// --- Forward declaration for the alert function ---
static void send_telegram_alert(const char *message);

// The event handler's only job is to set a bit in the event group.
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Retrying connection to the AP");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        // Signal that the connection is successful
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Function to initialize and start Wi-Fi connection
void wifi_init_sta(void) {
    wifi_event_group = xEventGroupCreate(); // Create the event group

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = { .sta = { .ssid = WIFI_SSID, .password = WIFI_PASSWORD, }, };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());
}

// Simple handler for HTTP events
esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    return ESP_OK;
}

// The function that performs the HTTPS request to Telegram
static void send_telegram_alert(const char *message) {
    char url[256];
    char encoded_message[128] = {0};

    // Manual URL encoding for spaces. This replaces the incorrect function call.
    int i = 0, j = 0;
    while (message[i] != '\0' && j < sizeof(encoded_message) - 4) { // Leave space for %20 and null terminator
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
        ESP_LOGI(TAG, "Telegram message sent successfully");
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

// This task waits for the signal and then sends the message.
static void wifi_connection_test_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Test task started, waiting for Wi-Fi connection signal...");
    
    // Wait here indefinitely until the WIFI_CONNECTED_BIT is set
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    
    // Once the bit is set, we know we are connected.
    ESP_LOGI(TAG, "Wi-Fi connection signal received! Sending test message...");
    send_telegram_alert("ESP32 Wi-Fi Connected!");

    ESP_LOGI(TAG, "Test complete. This task will now be deleted.");
    vTaskDelete(NULL); // Clean up and delete this task as its job is done.
}

// The main function of the application
void app_main(void)
{
    // Initialize NVS Flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize Wi-Fi
    wifi_init_sta();
    
    // Start the task that will wait for the connection and send the message
    xTaskCreate(&wifi_connection_test_task, "wifi_test_task", 4096, NULL, 5, NULL);
}