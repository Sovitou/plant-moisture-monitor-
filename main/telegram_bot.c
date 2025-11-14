#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "telegram_bot.h"

extern const char *BOT_TOKEN;
extern const char *CHAT_ID;

static const char *TAG = "TELEGRAM";

esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    return ESP_OK;
}

void telegram_send_message(const char *message)
{
    if (!message) return;
    char url[384];
    // naive encode: replace spaces with %%20 (we'll keep simple)
    char encoded[256];
    int i=0,j=0;
    while (message[i] && j < (int)sizeof(encoded)-4) {
        if (message[i] == ' ') { encoded[j++]='%'; encoded[j++]='2'; encoded[j++]='0'; }
        else encoded[j++]=message[i];
        i++;
    }
    encoded[j]=0;

    snprintf(url, sizeof(url), "https://api.telegram.org/bot%s/sendMessage?chat_id=%s&text=%s", BOT_TOKEN, CHAT_ID, encoded);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 15000,  // 15 second timeout
        .buffer_size = 2048,
        .buffer_size_tx = 2048,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Sent telegram message (status: %d)", status);
    } else {
        ESP_LOGE(TAG, "Failed to send telegram message: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

void telegram_start(void)
{
    // No polling needed - just alerts
    ESP_LOGI(TAG, "Telegram ready for sending alerts");
}

void telegram_stop(void)
{
    // Nothing to stop
}
