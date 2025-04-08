#pragma once

    #include <stdio.h>
    #include <stdint.h>
    #include <string.h>

    #include "esp_system.h"
    #include "nvs_flash.h"
    #include "esp_http_server.h"
    #include "mdns.h"
    #include "esp_log.h"

    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"

    #include "lwip/err.h"
    #include "lwip/sys.h"

#include "esp_littlefs_handler.c"


httpd_handle_t ESPserver = NULL;
struct ws_proj_data_st{
    bool cliConnected;
    bool newData;
    uint8_t ctrl_num;
    uint8_t wsData[16];
};
struct WEB_SOCKET_CLIENT{
    bool cliConnected;
    httpd_handle_t handle;
    int sockfd;
};

struct WEB_SOCKET_CLIENT UserClientWs;
struct ws_proj_data_st* ws_rx_data;

const char* WSTAG = "WS";

esp_err_t send_bin_via_ws(char type[4], uint8_t* data, size_t len){
    if(!UserClientWs.cliConnected) return ESP_FAIL;

    uint8_t* buffer = NULL;
    buffer = (uint8_t*)malloc(len+4);
    memcpy(buffer, type, 4);
    memcpy(buffer+4, data, len);

    httpd_ws_frame_t ws_tx_pkt;
    memset(&ws_tx_pkt, 0, sizeof(ws_tx_pkt));

    ws_tx_pkt.type = HTTPD_WS_TYPE_BINARY,
    ws_tx_pkt.payload = buffer;
    ws_tx_pkt.len = len+4;

    httpd_ws_send_frame_async(UserClientWs.handle, UserClientWs.sockfd, &ws_tx_pkt);
    
    free(buffer);
    return ESP_OK;
}


#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename){
    
    if (IS_FILE_EXT(filename, ".html")) 
        return httpd_resp_set_type(req, "text/html");
    else if (IS_FILE_EXT(filename, ".css")) 
        return httpd_resp_set_type(req, "text/css");
    else if (IS_FILE_EXT(filename, ".js")) 
        return httpd_resp_set_type(req, "application/javascript");
    else if (IS_FILE_EXT(filename, ".jpeg")) 
        return httpd_resp_set_type(req, "image/jpeg");
    else if (IS_FILE_EXT(filename, ".png")) 
        return httpd_resp_set_type(req, "image/png");
    else if (IS_FILE_EXT(filename, ".ico")) 
        return httpd_resp_set_type(req, "image/x-icon");
    else if (IS_FILE_EXT(filename, ".pdf")) 
        return httpd_resp_set_type(req, "application/pdf");
    
    return httpd_resp_set_type(req, "text/plain");
}
esp_err_t http_svr_handle_html(httpd_req_t* req){
    const char* html_file = "index.html";
    struct LITTLE_FS_FILE webFile = {NULL, 0};
    
    read_little_fs_file("/index.html", &webFile);

    if(webFile.data != NULL){
        set_content_type_from_file(req, html_file);
        httpd_resp_send(req, webFile.data, webFile.size);
        free(webFile.data);
    }
    else
        httpd_resp_send_404(req);

    return ESP_OK;
}

esp_err_t http_server_handle(httpd_req_t* req) {
    ESP_LOGI(WSTAG, "http server received req %d ! %s", req->method, req->uri);

    struct LITTLE_FS_FILE webFile = {NULL, 0};
    read_little_fs_file(req->uri, &webFile);

    if(webFile.data == NULL)
        httpd_resp_send_404(req);
    else{
        set_content_type_from_file(req, req->uri);
        httpd_resp_send(req, webFile.data, webFile.size);
        free(webFile.data);
    }

    return ESP_OK;
}

esp_err_t ws_server_txt_handle(httpd_ws_frame_t* ws_pkt){
    char ws_data_type[5];
    strlcpy(ws_data_type, (char*)ws_pkt->payload, sizeof(ws_data_type));

    if(strcmp(ws_data_type, "rest") == 0)
        esp_restart();
    else if(strcmp(ws_data_type, "ctrl") == 0){
        ws_rx_data->ctrl_num = ws_pkt->payload[4];
        ws_rx_data->newData = true;
    }else if(strcmp(ws_data_type, "data") == 0){
        memcpy(ws_rx_data->wsData, ws_pkt->payload + 4, ws_pkt->len - 4);
        ws_rx_data->newData = true;
    }else
        ESP_LOGI(WSTAG, "ws txt unknown : %s", (char*)ws_pkt->payload);

    return ESP_OK;
}

esp_err_t ws_server_handle(httpd_req_t* req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(WSTAG, "WEBSERVER CONNECTED WEBSOCKET!!!");
        UserClientWs.handle = req->handle;
        UserClientWs.sockfd = httpd_req_to_sockfd(req);
        ws_rx_data->cliConnected = true;
        UserClientWs.cliConnected = true;
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(ws_pkt));

    uint8_t* ws_buf = (uint8_t*)malloc(512);
    if(!ws_buf){
        ESP_LOGE(WSTAG, "ws buffer allocation error !!");
        return ESP_FAIL;
    }

    ws_pkt.payload = ws_buf;
    if(httpd_ws_recv_frame(req, &ws_pkt, 512) != ESP_OK){
        ESP_LOGE(WSTAG, "WebSocket frame receive failed");
        free(ws_buf);
        return ESP_FAIL;
    }

    if(ws_pkt.type == HTTPD_WS_TYPE_TEXT || ws_pkt.type == HTTPD_WS_TYPE_BINARY){
        ws_server_txt_handle(&ws_pkt);
    } else {
        ESP_LOGW(WSTAG, "unknown ws type: %d", ws_pkt.type);
    }

    free(ws_buf);
    return ESP_OK;
}

httpd_handle_t setup_server(void) {

    httpd_config_t http_cfg = HTTPD_DEFAULT_CONFIG();
    http_cfg.uri_match_fn = httpd_uri_match_wildcard;

    httpd_uri_t uri_ws = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_server_handle,
        .user_ctx = NULL,
        .is_websocket = true,
    };
    httpd_uri_t uri_get_html = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = http_svr_handle_html,
        .user_ctx = NULL,
    };
    httpd_uri_t uri_get = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = http_server_handle,
        .user_ctx = NULL,
    };


    if (httpd_start(&ESPserver, &http_cfg) == ESP_OK) {
        httpd_register_uri_handler(ESPserver, &uri_get_html);
        httpd_register_uri_handler(ESPserver, &uri_ws);
        httpd_register_uri_handler(ESPserver, &uri_get);
    }

    ESP_LOGI(WSTAG, "webserver setup successful !!");

    return ESPserver;
}

void init_mdns_service(const char* SERVER_MDNS_NAME) {
    esp_err_t err;

    err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGW("MDNS", "mdns server failed %d", err);
        return;
    }

    mdns_hostname_set(SERVER_MDNS_NAME);

    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
    mdns_service_add(NULL, "_ws", "_tcp", 80, NULL, 0);

    ESP_LOGI("MDNS", "Mdns hostname : %s", SERVER_MDNS_NAME);
}

esp_err_t init_ESP_server(bool* wifiConnected, const char* server_mdns_name, struct ws_proj_data_st* _ws_rx_data) {
    if (*wifiConnected) {
        ws_rx_data = _ws_rx_data;

        setup_server();
        init_mdns_service(server_mdns_name);

        little_fs_init();
        get_little_fs_info();


        ESP_LOGI(WSTAG, "WEB_SERVER SETUP SUCCESSFUL!!");
        return ESP_OK;
    }

    ESP_LOGI(WSTAG, "WIFI NOT CONNECTED !!!");
    return ESP_FAIL;
}


