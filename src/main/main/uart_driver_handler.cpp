#pragma once

extern "C" {
    #include <stdio.h>
    #include "driver/uart.h"
    #include "esp_log.h"
    #include "string.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

class ESP_IDF_UART_DRIVER{
private:
    int buadRate = 0;
    uint8_t txPin = 0, rxPin = 0;
    size_t bufferLen = 0;
    uart_port_t uPort;

public:
    uint8_t* rxBuffer = NULL;
    size_t rxBufLen = 0;

    ESP_IDF_UART_DRIVER(int baud = 115200, uint8_t tx = 17, uint8_t rx = 16, size_t len = 64, uart_port_t port = UART_NUM_1) : 
        buadRate(baud), txPin(tx), rxPin(rx), bufferLen(len), uPort(port){
    }
    ~ESP_IDF_UART_DRIVER(){
        if(uart_is_driver_installed(uPort)){
            uart_driver_delete(uPort);
        }
        if(this->rxBuffer != NULL)free(this->rxBuffer);
    }
    void init(){
        uart_config_t uartCfg = {
            .baud_rate = this->buadRate,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT,
        };

        uart_driver_install(uPort, bufferLen, bufferLen, 0, NULL, 0);
        uart_param_config(uPort, &uartCfg);
        uart_set_pin(uPort, this->txPin, this->rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        this->rxBuffer = (uint8_t*)malloc(bufferLen);
    }
    void write(const uint8_t* data, size_t len){
        uart_write_bytes(uPort, data, len);
        uart_wait_tx_done(uPort, pdMS_TO_TICKS(1000));
    }
    size_t read(uint16_t waitTime = 200){
        memset(this->rxBuffer, 0, bufferLen);
        rxBufLen = uart_read_bytes(uPort, this->rxBuffer, bufferLen, pdMS_TO_TICKS(waitTime)); // blocking function until data available
        return rxBufLen;
    }

};