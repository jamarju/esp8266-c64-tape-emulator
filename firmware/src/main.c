#include "espressif/esp_common.h"
#include "uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio.h" 

#include "i2s_freertos.h"
#include "draxcaught.h"

//extern void uart_div_modify(int,int);

#define LED 2

static inline uint8_t readRomByte(const uint8_t* addr)
{
    uint32_t bytes;
    bytes = *(uint32_t*)((uint32_t)addr & ~3);
    return ((uint8_t*)&bytes)[(uint32_t)addr & 3];
}

/*
* Simple task that prints "Hello world" every second
*/
void helloTask(void *pvParameters)
{
    int i = 0;
    // Config pin as GPIO12

    while(1) {
        printf("Hello world\n");
        GPIO_OUTPUT_SET(LED, i);
        i = !i;
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}


void ICACHE_FLASH_ATTR tapTask(void *pvParameters)
{
    uint32_t bitStream = 0;
    int bitStreamLen = 0;

    i2sInit();
    i2sSetRate(985248 / 8 / 2);    // bit rate = PAL / 8 (tap div) / 2 (our div), must be >40312

    while (1) {

        for (int a = 0x14; a < draxcaught_tap_len; a++) {
            int pulseLen = readRomByte(draxcaught_tap + a);
            if (pulseLen == 0) {
                a++;
                pulseLen = readRomByte(draxcaught_tap + a);
                a++;
                pulseLen |= readRomByte(draxcaught_tap + a) << 8;
                a++;
                pulseLen |= readRomByte(draxcaught_tap + a) << 16;
            }

            pulseLen /= 2;

            for (int b = 0; b < pulseLen; b++) {
                int outBit = 1; // reverse logic b/c we are driving through a transistor
                if (b > pulseLen / 2) outBit = 0;
                bitStream = (bitStream << 1) | outBit;
                bitStreamLen++;
                if (bitStreamLen == 32) {
                    bitStreamLen = 0;
                    i2sPushSample(bitStream);
                }

            }
        }   
    }
}

uint32
user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 8;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;

}

void
user_init(void)
{
    GPIO_ConfigTypeDef gpio;
    gpio.GPIO_IntrType = GPIO_PIN_INTR_DISABLE;
    gpio.GPIO_Mode = GPIO_Mode_Output;
    gpio.GPIO_Pin = GPIO_Pin_2;
    gpio.GPIO_Pullup = GPIO_PullUp_DIS;
    gpio_config(&gpio);


    // Set UART speed to 115200
    uart_div_modify(0, UART_CLK_FREQ / 115200);
    wifi_set_opmode(NULL_MODE);


    xTaskHandle t1, t2;
    xTaskCreate(helloTask, (const signed char *)"rx", 256, NULL, 2, &t1);
    xTaskCreate(tapTask, (const signed char *)"tap", 256, NULL, 2, &t2);

}

