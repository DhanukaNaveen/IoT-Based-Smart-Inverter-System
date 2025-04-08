extern "C" {
    #include <stdio.h>
    #include <stdint.h>
    #include <string.h>

    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/timers.h"

    #include "esp_system.h"
    #include "esp_log.h"
    #include "esp_timer.h"
    #include "driver/gpio.h"
    #include "driver/adc.h"

    #include "dht.h" // dht module lib

#include "esp_webSocServer_han.c"
}

#include "pzem004T_module_han.cpp"
#include "esp_wifi_driver.cpp"

////////////// PIN CONFIG ////////////////
#define UART2_TX 17
#define UART2_RX 16

#define RELAY_ON_LOGIC 0
#define RELAY_1_PIN GPIO_NUM_14
#define RELAY_2_PIN GPIO_NUM_27
#define RELAY_3_PIN GPIO_NUM_33
#define RELAY_4_PIN GPIO_NUM_32
#define RELAY_5_PIN GPIO_NUM_23

#define BATT_VOL_SEN_PIN 36
#define BATT_VOL_SEN_CH ADC1_CHANNEL_0
#define BATT_VOL_MAX 12.6f
#define BATT_VOL_MIN 10.6f

#define BUZZER_PIN GPIO_NUM_26
#define DHT11_SEN_PIN GPIO_NUM_25

//////////////////////////////////////////
// typedef enum {
//     ADC1_CHANNEL_0 = 0, /*!< ADC1 channel 0 is GPIO36 */
//     ADC1_CHANNEL_1,     /*!< ADC1 channel 1 is GPIO37 */
//     ADC1_CHANNEL_2,     /*!< ADC1 channel 2 is GPIO38 */
//     ADC1_CHANNEL_3,     /*!< ADC1 channel 3 is GPIO39 */
//     ADC1_CHANNEL_4,     /*!< ADC1 channel 4 is GPIO32 */
//     ADC1_CHANNEL_5,     /*!< ADC1 channel 5 is GPIO33 */
//     ADC1_CHANNEL_6,     /*!< ADC1 channel 6 is GPIO34 */
//     ADC1_CHANNEL_7,     /*!< ADC1 channel 7 is GPIO35 */
//     ADC1_CHANNEL_MAX,
// } adc1_channel_t;
/////////////SERVER CONFIG////////////////
const char* WIFI_SSID = "WiFi__5G";
const char* WIFI_PSWD = "#passwifi#";
const char* WEB_SVR_NAME = "espServer";
//////////////////////////////////////////

#define map_value(value, in_min, in_max, out_min, out_max) \
    (((value) - (in_min)) * ((out_max) - (out_min)) / ((in_max) - (in_min)) + (out_min))


struct DHT11_DATA_STRUCT{
    bool newdata;
    float temperature;
    float humidity;
};
struct BATT_DATA_STRUCT{
    float voltage;
    uint8_t precentage;
    uint8_t relay_ctrl_mode[5] = {2,2,2,2,2}; // 2 - auto | 1 - on | 0 - off
    uint8_t relay_off_precet[5];
};
struct PROJ_DATA_STRUCT {
    PZEM004T_DATA_STRUCT* energyModule;
    DHT11_DATA_STRUCT* dht11Module;
    BATT_DATA_STRUCT* battModule;
};
struct INVERTER_LIMITS_DATA{
    float AC_frq_dif[2] = {49.5f, 50.5f}; // Hz
    float AC_vol_dif[2] = {210,240}; // volt
    int8_t AC_cur_dif = 5; // A
    int8_t max_temp = 40; // C
    bool alarm_off = false; // overide alarm off
};

PROJ_DATA_STRUCT proj_data;
ws_proj_data_st ws_rx_pj_data;
ESP_WIFI_DRIVER wifi_drv;
INVERTER_LIMITS_DATA inverterLimits;

// relay control function
void control_relay(uint8_t relay_num, bool mode){
    switch (relay_num){
        case 1:
            gpio_set_direction(RELAY_1_PIN, GPIO_MODE_OUTPUT);
            gpio_set_level(RELAY_1_PIN, mode);
            break;
        case 2:
            gpio_set_direction(RELAY_2_PIN, GPIO_MODE_OUTPUT);
            gpio_set_level(RELAY_2_PIN, mode);
            break;
        case 3:
            gpio_set_direction(RELAY_3_PIN, GPIO_MODE_OUTPUT);
            gpio_set_level(RELAY_3_PIN, mode);
            break;
        case 4:
            gpio_set_direction(RELAY_4_PIN, GPIO_MODE_OUTPUT);
            gpio_set_level(RELAY_4_PIN, mode);
            break;
        case 5:
            gpio_set_direction(RELAY_5_PIN, GPIO_MODE_OUTPUT);
            gpio_set_level(RELAY_5_PIN, mode);
            break;
        
        default:
            break;
    }
            
    ESP_LOGI("RELAY", "Relay %d is set to %s", relay_num, mode ? "ON": "OFF");
}

// ebserver task
void WebServer_tsk(void *arg){
    wifi_drv.wifi_init();
    wifi_drv.wifi_connect(WIFI_SSID, WIFI_PSWD);

    memset(&ws_rx_pj_data, 0, sizeof(ws_rx_pj_data));
    init_ESP_server(&wifi_drv.iswifi_sta_connected, WEB_SVR_NAME, &ws_rx_pj_data);


    while (1){
        /// receiving data hanlde
        if(ws_rx_pj_data.newData && ws_rx_pj_data.wsData[0] != 0){
            ws_rx_pj_data.newData = false;

            printf("ws rx data: %s\n", (char*)ws_rx_pj_data.wsData);
            
            if(strnstr((char*)ws_rx_pj_data.wsData, "rely", 4) != NULL){
                proj_data.battModule->relay_ctrl_mode[ws_rx_pj_data.wsData[4]] = ws_rx_pj_data.wsData[5];
                proj_data.battModule->relay_off_precet[ws_rx_pj_data.wsData[4]] = ws_rx_pj_data.wsData[5];

                if(ws_rx_pj_data.wsData[5] != 2){
                    control_relay(ws_rx_pj_data.wsData[4], ws_rx_pj_data.wsData[5]);
                }
                ESP_LOGI("WS","relay %d setted to %d", ws_rx_pj_data.wsData[4], ws_rx_pj_data.wsData[5]);
            }
            else if(strnstr((char*)ws_rx_pj_data.wsData, "alam", 4) != NULL)
                inverterLimits.alarm_off =  !inverterLimits.alarm_off;

            memset(ws_rx_pj_data.wsData, 0, sizeof(ws_rx_pj_data.wsData));
        }

        if(ws_rx_pj_data.cliConnected){
            /// sending data handle
            // send ac
            uint8_t wsSendDataBuf[32];
            memcpy(wsSendDataBuf, proj_data.energyModule, 24);
            send_bin_via_ws("ACDT", wsSendDataBuf, sizeof(wsSendDataBuf));
            // send batt
            vTaskDelay(pdMS_TO_TICKS(50));
            memset(wsSendDataBuf, 0, sizeof(wsSendDataBuf));
            memcpy(wsSendDataBuf, proj_data.battModule, 10);
            memcpy(wsSendDataBuf + 10, &proj_data.dht11Module->temperature, 4);
            // memcpy(wsSendDataBuf + 9, &proj_data.battModule->relay_ctrl_mode, 5);
            send_bin_via_ws("BTDT", wsSendDataBuf, sizeof(wsSendDataBuf));
        }
        
        vTaskDelay(500);
    }
}

void EnergyMod_tsk(void *arg){
    static PZEM004T_DATA_STRUCT EnergyData;
    pzem004T_module_han EnergyMod(&EnergyData, UART2_TX, UART2_RX);

    EnergyMod.init();

    proj_data.energyModule = &EnergyData;

    vTaskDelay(pdMS_TO_TICKS(4000));

    while (1){
        printf("Reading Energy meter data...\n");

        if(EnergyMod.read_main_values())
            printf("Energy meter data: V: %0.1f V A: %0.3f A P: %.1f W E: %.1f Wh Frq: %.1f Hz P.F.: %.2f\n",
            EnergyData.voltage, EnergyData.current, EnergyData.power, EnergyData.energy, EnergyData.freq, EnergyData.powerFactor);
        else
            printf("energy meter module not detected !!\n");

        vTaskDelay(pdMS_TO_TICKS(500));
    }   
}

void Dht11_sen_tsk(void *arg){
    static DHT11_DATA_STRUCT dht11_data_store;
    vTaskDelay(3000);

    proj_data.dht11Module = &dht11_data_store;

    while (1){
        if(dht_read_float_data(DHT_TYPE_DHT11, DHT11_SEN_PIN, &dht11_data_store.humidity, &dht11_data_store.temperature) == ESP_OK){
            ESP_LOGI("DHT11", "dht read successful! temp: %.2f RH: %.2f", dht11_data_store.temperature, dht11_data_store.humidity);
        }else {
            ESP_LOGE("DHT11", "dht11 data read error!!");
        }

        vTaskDelay(5000);
    }
}

void batt_vol_tsk(void *arg){
    static BATT_DATA_STRUCT battData;
    proj_data.battModule = &battData;

    gpio_config_t pinCfg = {
        .pin_bit_mask = (1ULL << BATT_VOL_SEN_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&pinCfg);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(BATT_VOL_SEN_CH, ADC_ATTEN_DB_12);

    vTaskDelay(pdMS_TO_TICKS(4000));

    while (1){
        float voltage = adc1_get_raw(BATT_VOL_SEN_CH);
        voltage = voltage * 3.3f / 4096.0f;
        voltage = voltage * (30.0f + 7.5f)/ 7.5f;

        battData.voltage = voltage;
        battData.precentage = (voltage < BATT_VOL_MIN) ? 0 : map_value(voltage, BATT_VOL_MIN, BATT_VOL_MAX, 0, 100);
        ESP_LOGI("BAT", "battery voltage: %.3f | %d", battData.voltage, battData.precentage);

        
        for(uint8_t i = 0; i < 5; i++){
            if(battData.precentage < battData.relay_off_precet[i] && battData.relay_ctrl_mode[i] == 2)
                control_relay(i+1, !RELAY_ON_LOGIC);
            else if(battData.relay_ctrl_mode[i] == 2)
                control_relay(i+1, RELAY_ON_LOGIC);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/// task handlers
TaskHandle_t EnergyMod_tsk_h = NULL, WebServer_tsk_h = NULL, Dht11_sen_tsk_h = NULL, batt_vol_tsk_h = NULL;
TimerHandle_t buzzer_timer_h;

static volatile bool buzz_active = true;
static void IRAM_ATTR buzz_isr_h(TimerHandle_t xTimer){
    xTimerStop(xTimer, 0);
    gpio_set_level(BUZZER_PIN, 0);
    buzz_active = false;
}
void buzz_beep(uint16_t tms){
    if(buzz_active)return;

    buzz_active = true;
    gpio_set_level(BUZZER_PIN, 1);
    xTimerChangePeriod(buzzer_timer_h, pdMS_TO_TICKS(tms), 0);
    xTimerStart(buzzer_timer_h, 0);
}
void buz_pin_init(){
    gpio_config_t buz_pin_conf{
        .pin_bit_mask = (1ULL << BUZZER_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&buz_pin_conf);

    gpio_set_level(BUZZER_PIN, 1);
    
    buzzer_timer_h = xTimerCreate("buzzer_timer_h", pdMS_TO_TICKS(100), pdTRUE, NULL, buzz_isr_h);
    if(buzzer_timer_h != NULL) xTimerStart(buzzer_timer_h, 0);
}

bool buzzer_alarm_handler(){
    if(proj_data.energyModule->current > inverterLimits.AC_cur_dif) return true;
    else if(proj_data.energyModule->voltage < inverterLimits.AC_vol_dif[0] ||
            proj_data.energyModule->voltage > inverterLimits.AC_vol_dif[1]) return true;
    else if(proj_data.energyModule->freq < inverterLimits.AC_frq_dif[0] ||
            proj_data.energyModule->freq > inverterLimits.AC_frq_dif[1]) return true;
    else if(proj_data.dht11Module->temperature > inverterLimits.max_temp) return true;
    else return false;
}

extern "C" void app_main(void){
    printf("booting...\n");
    buz_pin_init();

    xTaskCreatePinnedToCore(EnergyMod_tsk, "EnergyMod_tsk", 1024*2, NULL, 5, &EnergyMod_tsk_h, 1);
    xTaskCreate(WebServer_tsk, "WebServer_tsk", 1024*8, NULL, 5, &WebServer_tsk_h);
    xTaskCreate(Dht11_sen_tsk, "Dht11_sen_tsk", 1024*2, NULL, 5, &Dht11_sen_tsk_h);
    xTaskCreate(batt_vol_tsk, "batt_vol_tsk", 1024*4, NULL, 5, &batt_vol_tsk_h);

    
    buzz_beep(50);

    bool alarm_status = false;
    while (1){

        while ((alarm_status = buzzer_alarm_handler()) & !inverterLimits.alarm_off){
            ESP_LOGW("ALARM", "ALARM ON!!");
            buzz_beep(100);
            vTaskDelay(100);
            buzz_beep(100);
            vTaskDelay(100);
            buzz_beep(200);
            vTaskDelay(800);
        }
        if(!alarm_status && inverterLimits.alarm_off) inverterLimits.alarm_off = false;
        
        switch (ws_rx_pj_data.ctrl_num){
            case 0:
                break;
            default:
                break;
        }

        vTaskDelay(100);
    }
    
}