#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_system.h"
#include "esp_log.h"
#include "esp_littlefs.h"
#include "esp_spi_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

const char* LFSTAG = "LFS";

esp_err_t little_fs_init(){
    ESP_LOGI(LFSTAG, "Initializing LittelFS");

    esp_vfs_littlefs_conf_t lfs_cfg = {
        .base_path = "/littlefs",
        .partition_label = "littlefs",
        .format_if_mount_failed = true,
        .read_only = true,
        .dont_mount = false,
    };
    esp_err_t ret = esp_vfs_littlefs_register(&lfs_cfg);
    if(ret == ESP_OK);
    else if(ret == ESP_FAIL)
        ESP_LOGE(LFSTAG, "Failed to mount or format filesystem");
    else if(ret == ESP_ERR_NOT_FOUND)
        ESP_LOGE(LFSTAG, "Failed to find LittleFS partition");
    else
        ESP_LOGE(LFSTAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));

    return ret;
}
esp_err_t little_fs_deinit(){
    ESP_LOGI(LFSTAG,"LITTLEFS DEINIT");
    return esp_vfs_littlefs_unregister("littlefs");
}
esp_err_t get_little_fs_info(){
    size_t total = 0, used = 0;
    esp_err_t ret = esp_littlefs_info("littlefs", &total, &used);

    if(ret == ESP_OK)
        ESP_LOGI(LFSTAG, "Partition size: total: %d, used: %d", total, used);
    else
        ESP_LOGE(LFSTAG, "Failed to mount or format filesystem");
    
    return ret;
}

struct LITTLE_FS_FILE{
    char* data;
    size_t size;
};

void read_little_fs_file(const char* file_path, struct LITTLE_FS_FILE* lfsFile){
    char lfs_f_path[64] = {0};
    snprintf(lfs_f_path, 32, "/littlefs%s", file_path);

    // struct LITTLE_FS_FILE lfsFile = {NULL,0};

    struct stat st;
    if(stat(lfs_f_path, &st) == 0)
        lfsFile->size = st.st_size;
    else{
        ESP_LOGI(LFSTAG, "read file not found !!");
        // return NULL;
    }

    if(lfsFile->size == 0){
        ESP_LOGI(LFSTAG, "read file not found !!");
        // return NULL;
    }

    ESP_LOGI(LFSTAG, "read file : %s | size : %d", lfs_f_path, lfsFile->size);

    FILE *f = fopen(lfs_f_path, "r");
    if (!f) {
        ESP_LOGE(LFSTAG, "Failed to read existing file : %s", lfs_f_path);
        // return NULL;
    }

    lfsFile->data = (char*)malloc(lfsFile->size);
    fread(lfsFile->data, 1, lfsFile->size, f);
    fclose(f);
    
    return;
}
