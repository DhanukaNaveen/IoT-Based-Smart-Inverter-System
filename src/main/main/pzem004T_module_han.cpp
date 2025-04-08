#pragma once

#include "uart_driver_handler.cpp"

#define XUINT_16(h,l) (h << 8 | l)
#define XUINT_32(a,b,c,d) (a << 24 | b << 16 | c << 8 | d)

#define FUN_READ_HOLDING_REG 0x03
#define FUN_READ_INPUT_REG 0x04
#define FUN_WRITE_SINGLE_REG 0x06
#define FUN_CALIBRATION 0x06
#define FUN_RESET_ENERGY 0x06

//////////// reading registers ////////////
// 0x0000          -- volatage -- V x 0.1
// 0x0001 - 0x0002 -- current  -- A x 0.001
// 0x0003 - 0x0004 -- power    -- W x 0.1
// 0x0005 - 0x0006 -- energy   -- Wh x 1
// 0x0007          -- frequency-- Hz x 0.1
// 0x0008          -- pf       -- value x 0.01
// 0x0009          -- alarm    -- 0xFFFF = on|0x0000 = off

typedef struct PZEM004T_DATA_STRUCT {
    float voltage = 0;
    float current = 0;
    float power = 0;
    float energy = 0;
    float freq = 0;
    float powerFactor = 0;
    bool newData = false;
    bool isConnected = false;
};


class pzem004T_module_han  {
private:
    ESP_IDF_UART_DRIVER xuart;
    PZEM004T_DATA_STRUCT *dataStore;
    
    const uint8_t mainDataReq[8] = {0xf8,0x04,0x00,0x00,0x00,0x0a,0x64,0x64};  // for all 10 registers
    // const uint8_t mainDataReq[8] = {0xf8,0x04,0x00,0x00,0x00,0x07,0xa5,0xa1}; // only main registers
    const uint8_t resetPwrReq[8] = {0xf8,0x42,0x00,0x00,0x00,0x00,0x6d,0xac};


    // uint16_t check_crc(uint8_t *data = NULL, uint8_t len = 0){
    //     uint16_t crc = 0xFFFF; // Initial value
    //     uint16_t constpoly = 0xA001; // Polynomial X16 + X15 + X2 + 1

    //     for (uint16_t i = 0; i < len; i++){
    //         crc ^= data[i];
    //         for(uint8_t j = 0; j < 8; j++){
    //             if(crc & 0x0001) crc = (crc >> 1) ^ constpoly;
    //             else crc >>= 1;
    //         }
    //     }
    //     return crc;
    // }
    bool constructData(const uint8_t* data){
        if(xuart.rxBuffer[0] == 0xf8 && xuart.rxBuffer[1] == 0x04){
            dataStore->voltage = XUINT_16(data[3],data[4]) * 0.1f;
            dataStore->current = XUINT_32(data[7],data[8],data[5],data[6]) * 0.001f;
            dataStore->power = XUINT_32(data[11],data[12],data[9],data[10]) * 0.1f;
            dataStore->energy = XUINT_32(data[15],data[16],data[13],data[14]) * 0.001f;
            dataStore->freq = XUINT_16(data[17],data[18]) * 0.1f;
            dataStore->powerFactor = XUINT_16(data[19],data[20]) * 0.01f;
            dataStore->newData = true;
            return true;
        }else {
            dataStore->newData = false;
            return false;
        }
    }
public:
    pzem004T_module_han(PZEM004T_DATA_STRUCT *datastore, uint8_t txPin, uint8_t rxPin) : xuart(9600, txPin, rxPin, 256, UART_NUM_1), dataStore(datastore){
    }
    ~pzem004T_module_han();

    void init(){
        xuart.init();
        memset((void*)dataStore, 0, sizeof(*dataStore));
    }
    
    bool read_main_values(){
        xuart.write(mainDataReq, sizeof(mainDataReq));

        uint8_t byteLen = xuart.read();

        if(byteLen > 0){
            dataStore->isConnected = true;
            return this->constructData(xuart.rxBuffer);
        }
        dataStore->isConnected = false;
        return false;
    }
    bool reset_energyMeter(){
        xuart.write(resetPwrReq, sizeof(resetPwrReq));
        if(xuart.read()){
            if(xuart.rxBuffer[1] == 0x42) return true;
        }
        return false;
    }
};

