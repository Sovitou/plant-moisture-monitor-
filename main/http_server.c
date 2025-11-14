#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "monitor.h"

static const char *TAG = "HTTP_SERVER";

static esp_err_t status_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET /status");
    int val = monitor_get_last_value();
    bool running = monitor_is_running();
    char resp[128];
    snprintf(resp, sizeof(resp), "{\"running\":%s,\"last_value\":%d}", running?"true":"false", val);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t start_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "POST /start - Starting monitor");
    monitor_start();
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

static esp_err_t stop_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "POST /stop - Stopping monitor");
    monitor_stop();
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

httpd_handle_t start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.ctrl_port = 32768;
    
    httpd_handle_t server = NULL;
    esp_err_t err = httpd_start(&server, &config);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP server started on port %d", config.server_port);
        
        httpd_uri_t status_uri = {
            .uri = "/status",
            .method = HTTP_GET,
            .handler = status_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &status_uri);
        ESP_LOGI(TAG, "Registered GET /status");

        httpd_uri_t start_uri = {
            .uri = "/start",
            .method = HTTP_POST,
            .handler = start_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &start_uri);
        ESP_LOGI(TAG, "Registered POST /start");

        httpd_uri_t stop_uri = {
            .uri = "/stop",
            .method = HTTP_POST,
            .handler = stop_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &stop_uri);
        ESP_LOGI(TAG, "Registered POST /stop");
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(err));
    }
    return server;
}
