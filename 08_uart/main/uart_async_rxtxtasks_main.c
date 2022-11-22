/*
 * @Author: Fairy 2754283833@qq.com
 * @Date: 2022-11-20 11:05:35
 * @LastEditTime: 2022-11-22 23:12:12
 * @LastEditors: Fairy
 * @Description:
 * @FilePath: \uart_async_rxtxtasks_ws\main\uart_async_rxtxtasks_main.c
 * Copyright (c) 2022 by Fairy 2754283833@qq.com, All Rights Reserved.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

#include "driver/rmt.h"
#include "led_strip.h"

#include "freertos/queue.h"

// for ws2812
#define RMT_TX_CHANNEL RMT_CHANNEL_0
#define WS2812_PIN 38
#define WS2812_NUM 1
#define WS2812_LIGHT 50

// for uart_1
static const int RX_BUF_SIZE = 1024;

#define TXD_PIN (GPIO_NUM_4) // to  B7
#define RXD_PIN (GPIO_NUM_5) //   B6

/**
 * @description: init uart_1
 * @return      {*}
 */
void init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

int sendData(const char *logName, const char *data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

/**
 * @description: uart_1 transmit task 串口1发送任务
 * @return      {*}
 * @param {void} *arg
 */
static void tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1)
    {
        sendData(TX_TASK_TAG, "led on"); // important 重要函数 发送
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        sendData(TX_TASK_TAG, "led off"); // important 重要函数 发送
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/**
 * @description: uart_1 receive task 串口1接收任务
 * @return      {*}
 * @param {void} *arg
 */
static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1); // 申请一块内存空间 与free()成对出现

    while (1)
    {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS); // important 重要函数 接收
        if (rxBytes > 0)
        {
            data[rxBytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
    }
    free(data);
}

/**
 * @description: WS2812 task
 * @return      {*}
 * @param {void} *arg
 */
static void ws_2812_task(void *arg)
{

    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;

    uint8_t ws2812_dat = 0;

    xQueueHandle xQueue = (xQueueHandle)arg;

    //////// ws2812 init begin
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(WS2812_PIN, RMT_TX_CHANNEL);
    // set counter clock to 40MHz
    config.clk_div = 2;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(WS2812_NUM, (led_strip_dev_t)config.channel);
    led_strip_t *strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip)
    {
        ESP_LOGE("WS2812", "install WS2812 driver failed");
    }
    // Clear LED strip (turn off all LEDs)
    ESP_ERROR_CHECK(strip->clear(strip, 100));
    //////// ws2812 init finish

    while (1)
    {
        if (xQueueReceive(xQueue, &ws2812_dat, 10) != pdPASS)
        {
            if (ws2812_dat == 1)
            {
                red = WS2812_LIGHT;
                green = 0;
                blue = 0;
            }
            else if (ws2812_dat == 2)
            {
                red = 0;
                green = WS2812_LIGHT;
                blue = 0;
            }
            else if (ws2812_dat == 3)
            {
                red = 0;
                green = 0;
                blue = WS2812_LIGHT;
            }
            else if (ws2812_dat == 4)
            {
                red = 0;
                green = 0;
                blue = 0;
            }
            else if (ws2812_dat == 5)
            {
                red = 46;
                green = 49;
                blue = 124;
            }
            else if (ws2812_dat == 6)
            {
                red = 23;
                green = 114;
                blue = 180;
            }
            ESP_ERROR_CHECK(strip->set_pixel(strip, 0, red, green, blue));
            ESP_ERROR_CHECK(strip->refresh(strip, 100));
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

/**
 * @description: uart_0 receive task
 * @return      {*}
 * @param {void} *arg
 */
static void u0_rx_task(void *arg)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_0, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, 43, 44, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    xQueueHandle xQueue = (xQueueHandle)arg;

    uint8_t ws2812_cnt = 0;
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);

    while (1)
    {
        const int rxBytes = uart_read_bytes(UART_NUM_0, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0)
        {
            data[rxBytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);

            ws2812_cnt = 0;

            if (strstr((char *)data, "red") != NULL)
            {
                ws2812_cnt = 1;
            }
            else if (strstr((char *)data, "green") != NULL)
            {
                ws2812_cnt = 2;
            }
            else if (strstr((char *)data, "blue") != NULL)
            {
                ws2812_cnt = 3;
            }
            else if (strstr((char *)data, "clear") != NULL)
            {
                ws2812_cnt = 4;
            }
            else if (strstr((char *)data, "purple") != NULL)
            {
                ws2812_cnt = 5;
            }
            else if (strstr((char *)data, "qing") != NULL)
            {
                ws2812_cnt = 6;
            }
            if (ws2812_cnt != 0)
            {
                if (xQueueSend(xQueue, (void *)&ws2812_cnt, 10) != pdPASS)
                {
                    ESP_LOGE("rx_task", "xQueueSend failed");
                }
            }

            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
    }
    free(data);
}

void app_main(void)
{

    static xQueueHandle xQueue_UART2WS2812 = NULL;

    init();

    xQueue_UART2WS2812 = xQueueCreate(10, sizeof(uint8_t));
    if (xQueue_UART2WS2812 == NULL)
    {
        ESP_LOGE("app_main", "The queue could not be created");
    }

    xTaskCreate(rx_task, "uart_rx_task", 1024 * 5, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(u0_rx_task, "u0_rx_task", 1024 * 5, (void *)xQueue_UART2WS2812, configMAX_PRIORITIES, NULL);
    xTaskCreate(tx_task, "uart_tx_task", 1024 * 5, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(ws_2812_task, "ws_2812_task", 1024 * 10, (void *)xQueue_UART2WS2812, configMAX_PRIORITIES - 2, NULL);
}
