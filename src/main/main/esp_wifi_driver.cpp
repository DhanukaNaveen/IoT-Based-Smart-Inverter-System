#pragma once

#include "stdio.h"
#include "string.h"

#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

const char* WTAG = "WIFI";

#define WIFI_RETRY_ATTEMPTS 50
#define WIFI_AUTHMODE WIFI_AUTH_WPA2_PSK
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

/// should run in sperated rtos task

class ESP_WIFI_DRIVER {
private:

    uint8_t wifi_retry_count = 0;
    esp_netif_t *wifi_netif = NULL;
    EventGroupHandle_t s_wifi_event_group = NULL;
    esp_event_handler_instance_t ip_event_handler;
    esp_event_handler_instance_t wifi_event_handler;

    static void ip_event_cb(void *arg, esp_event_base_t e_base, int32_t e_id, void *e_data);
    static void wifi_event_cb(void *arg, esp_event_base_t e_base, int32_t e_id, void *e_data);

public:
    wifi_ap_record_t wifi_ap_info;
    bool iswifi_sta_connected = false;

    ESP_WIFI_DRIVER();
    ~ESP_WIFI_DRIVER();

    esp_err_t wifi_init();
    esp_err_t wifi_connect(const char* wifi_ssid, const char* wifi_pswd);
    esp_err_t wifi_disconnect();
    esp_err_t wifi_deinit();

    esp_err_t get_wifi_sta_info(){
        esp_err_t ret = esp_wifi_sta_get_ap_info(&this->wifi_ap_info);
        if(ret == ESP_ERR_WIFI_CONN){
            ESP_LOGE(WTAG, "Wi-Fi station interface not initialized");
        }
        else if(ret == ESP_ERR_WIFI_NOT_CONNECT){
            ESP_LOGE(WTAG, "Wi-Fi station is not connected");
        }
        return ret;
    }

    int get_wifi_sta_rssi(){
        int wrssi;
        esp_wifi_sta_get_rssi(&wrssi);
        return wrssi;
    }
    
};

ESP_WIFI_DRIVER::ESP_WIFI_DRIVER(){
}

ESP_WIFI_DRIVER::~ESP_WIFI_DRIVER() {
}

void ESP_WIFI_DRIVER::ip_event_cb(void *arg, esp_event_base_t e_base, int32_t e_id, void *e_data){
    ESP_WIFI_DRIVER* wifi_drv_cls = (ESP_WIFI_DRIVER *)arg;

    ESP_LOGI(WTAG, "Handling IP event, event code 0x%" PRIx32, e_id);
    
    switch (e_id){

        case (IP_EVENT_STA_GOT_IP):{
            ip_event_got_ip_t *event_ip = (ip_event_got_ip_t *)e_data;
            ESP_LOGI(WTAG, "Got IP: " IPSTR, IP2STR(&event_ip->ip_info.ip));
            wifi_drv_cls->wifi_retry_count = 0;
            xEventGroupSetBits(wifi_drv_cls->s_wifi_event_group, WIFI_CONNECTED_BIT);
            }
            break;
        case (IP_EVENT_STA_LOST_IP):
            ESP_LOGI(WTAG, "Lost IP");
            break;
        case (IP_EVENT_GOT_IP6):{
            ip_event_got_ip6_t *event_ip6 = (ip_event_got_ip6_t *)e_data;
            ESP_LOGI(WTAG, "Got IPv6: " IPV6STR, IPV62STR(event_ip6->ip6_info.ip));
            wifi_drv_cls->wifi_retry_count = 0;
            xEventGroupSetBits(wifi_drv_cls->s_wifi_event_group, WIFI_CONNECTED_BIT);
            }
            break;
        default:
            ESP_LOGI(WTAG, "IP event not handled");
            break;
    }
}

void ESP_WIFI_DRIVER::wifi_event_cb(void *arg, esp_event_base_t e_base, int32_t e_id, void *e_data){
    ESP_WIFI_DRIVER* wifi_drv_cls = (ESP_WIFI_DRIVER *)arg;

    ESP_LOGI(WTAG, "Handling Wi-Fi event, event code 0x%" PRIx32, e_id);

    switch (e_id){
        case (WIFI_EVENT_WIFI_READY):
            ESP_LOGI(WTAG, "Wi-Fi ready");
            break;
        case (WIFI_EVENT_SCAN_DONE):
            ESP_LOGI(WTAG, "Wi-Fi scan done");
            break;
        case (WIFI_EVENT_STA_START):
            ESP_LOGI(WTAG, "Wi-Fi started, connecting to AP...");
            esp_wifi_connect();
            break;
        case (WIFI_EVENT_STA_STOP):
            ESP_LOGI(WTAG, "Wi-Fi stopped");
            wifi_drv_cls->iswifi_sta_connected = false;
            break;
        case (WIFI_EVENT_STA_CONNECTED):
            ESP_LOGI(WTAG, "Wi-Fi connected");
            wifi_drv_cls->iswifi_sta_connected = true;
            break;
        case (WIFI_EVENT_STA_DISCONNECTED):
            ESP_LOGI(WTAG, "Wi-Fi disconnected");
            wifi_drv_cls->iswifi_sta_connected = false;

            if (wifi_drv_cls->wifi_retry_count < WIFI_RETRY_ATTEMPTS) {
                vTaskDelay(pdMS_TO_TICKS(200));
                ESP_LOGI(WTAG, "Retrying to connect to Wi-Fi network...");
                esp_wifi_connect();
                wifi_drv_cls->wifi_retry_count++;
            } else {
                ESP_LOGI(WTAG, "Failed to connect to Wi-Fi network");
                wifi_drv_cls->iswifi_sta_connected = false;
                xEventGroupSetBits(wifi_drv_cls->s_wifi_event_group, WIFI_FAIL_BIT);
            }
            break;
        case (WIFI_EVENT_STA_AUTHMODE_CHANGE):
            ESP_LOGI(WTAG, "Wi-Fi authmode changed");
            break;
        default:
            ESP_LOGI(WTAG, "Wi-Fi event not handled");
            break;
    }
}

esp_err_t ESP_WIFI_DRIVER::wifi_init(){
    esp_err_t ret;

    ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        nvs_flash_erase();
        ret = nvs_flash_init();
    }

    this->s_wifi_event_group = xEventGroupCreate();

    ret = esp_netif_init();
    if(ret != ESP_OK){
        return ret;
    }

    ret = esp_event_loop_create_default();
    if(ret != ESP_OK){
        return ret;
    }

    ret = esp_wifi_set_default_wifi_sta_handlers();
    if(ret != ESP_OK){
        return ret;
    }

    this->wifi_netif = esp_netif_create_default_wifi_sta();
    if(this->wifi_netif == NULL){
        return ESP_FAIL;
    }

    // Wi-Fi stack configuration parameters
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &ESP_WIFI_DRIVER::wifi_event_cb, this, &this->wifi_event_handler);
    esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ESP_WIFI_DRIVER::ip_event_cb, this, &this->ip_event_handler);

    return ret;
}

esp_err_t ESP_WIFI_DRIVER::wifi_connect(const char* wifi_ssid, const char* wifi_pswd){
    wifi_config_t wifi_cfg = {
        .sta = {
            // this sets the weakest authmode accepted in fast scan mode (default)
            .threshold{
                .authmode = WIFI_AUTHMODE,
            },
        },
    };
    strncpy((char*)wifi_cfg.sta.ssid, wifi_ssid, sizeof(wifi_cfg.sta.ssid));
    strncpy((char*)wifi_cfg.sta.password, wifi_pswd, sizeof(wifi_cfg.sta.password));

    esp_wifi_set_ps(WIFI_PS_NONE); // Wi-Fi power save type
    esp_wifi_set_storage(WIFI_STORAGE_RAM); // Wi-Fi storage type

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);

    ESP_LOGI(WTAG, "Connecting to Wi-Fi network: %s", wifi_cfg.sta.ssid);
    esp_wifi_start();

    EventBits_t bits = xEventGroupWaitBits(this->s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if(bits & WIFI_CONNECTED_BIT){
        ESP_LOGI(WTAG, "Connected to Wi-Fi network: %s", wifi_cfg.sta.ssid);
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT){
        ESP_LOGE(WTAG, "Failed to connect to Wi-Fi network: %s", wifi_cfg.sta.ssid);
        return ESP_FAIL;
    }

    ESP_LOGE(WTAG, "Unexpected Wi-Fi error");
    return ESP_FAIL;
}

esp_err_t ESP_WIFI_DRIVER::wifi_disconnect(){
    if(this->s_wifi_event_group){
        vEventGroupDelete(this->s_wifi_event_group);
    }
    return esp_wifi_disconnect();
}

esp_err_t ESP_WIFI_DRIVER::wifi_deinit(){
    esp_err_t ret;
    ret = esp_wifi_stop();
    if(ret == ESP_ERR_WIFI_NOT_INIT){
        ESP_LOGE(WTAG, "Wi-Fi stack not initialized");
        return ret;
    }

    esp_wifi_deinit();
    esp_wifi_clear_default_wifi_driver_and_handlers(this->wifi_netif);
    esp_netif_destroy(this->wifi_netif);

    esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, this->ip_event_handler);
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, this->wifi_event_handler);

    return ESP_OK;
}