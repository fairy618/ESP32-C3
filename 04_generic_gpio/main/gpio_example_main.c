/*
 * @Author: Fairy 2754283833@qq.com
 * @Date: 2022-08-03 18:43:28
 * @LastEditTime: 2022-08-06 16:02:01
 * @LastEditors: Fairy
 * @Description: 
 * @FilePath: \generic_gpio\main\gpio_example_main.c
 * Copyright (c) 2022 by Fairy 2754283833@qq.com, All Rights Reserved. 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

/**
 * Brief:
 * This test code shows how to configure gpio and how to use gpio interrupt.
 *
 * GPIO status:
 * GPIO18: output
 * GPIO19: output
 * GPIO4:  input, pulled up, interrupt from rising edge and falling edge
 * GPIO5:  input, pulled up, interrupt from rising edge.
 *
 * Test:
 * Connect GPIO18 with GPIO4
 * Connect GPIO19 with GPIO5
 * Generate pulses on GPIO18/19, that triggers interrupt on GPIO4/5
 *
 */

#define GPIO_OUTPUT_IO_0 18
#define GPIO_OUTPUT_IO_1 19
#define GPIO_OUTPUT_PIN_SEL ((1ULL << GPIO_OUTPUT_IO_0) | (1ULL << GPIO_OUTPUT_IO_1))
#define GPIO_INPUT_IO_0 4
#define GPIO_INPUT_IO_1 5
#define GPIO_INPUT_PIN_SEL ((1ULL << GPIO_INPUT_IO_0) | (1ULL << GPIO_INPUT_IO_1))
#define ESP_INTR_FLAG_DEFAULT 0

// 用户按键
#define KEY1 13
#define KEY2 14
#define KEY3 21

// 队列，用于存储从中断中发送的数据
static xQueueHandle gpio_evt_queue = NULL;

// 中断函数，发送数据到队列中
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// 任务，从队列中接受数据
static void gpio_task_example(void *arg)
{
    uint32_t io_num;
    int io_status;
    for (;;)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            io_status = gpio_get_level(io_num);
            if (io_num == KEY1)
            {
                printf("KEY1 is %s.\n", ((io_status == 1) ? "loosen" : "press"));
            }
            else if (io_num == KEY2)
            {
                printf("KEY2 is %s.\n", ((io_status == 1) ? "loosen" : "press"));
            }
            else if (io_num == KEY3)
            {
                printf("KEY3 is %s.\n", ((io_status == 1) ? "loosen" : "press"));
            }
            else
            {
                printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
            }
        }
    }
}

void app_main(void)
{
    // zero-initialize the config structure.
    gpio_config_t io_conf = {};
    // disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    // disable pull-down mode
    io_conf.pull_down_en = 0;
    // disable pull-up mode
    io_conf.pull_up_en = 0;
    // configure GPIO with the given settings
    gpio_config(&io_conf);

    // interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    // bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

//***************************************************
// 按键对应的GPIO复位
    gpio_reset_pin(KEY1);
    gpio_reset_pin(KEY2);
    gpio_reset_pin(KEY3);

// 设置按键的GPIO为输入
    gpio_set_direction(KEY1, GPIO_MODE_INPUT);
    gpio_set_direction(KEY2, GPIO_MODE_INPUT);
    gpio_set_direction(KEY3, GPIO_MODE_INPUT);

// 设置按键的GPIO的中断类型：上升沿和下降沿
    gpio_set_intr_type(KEY1, GPIO_INTR_ANYEDGE);
    gpio_set_intr_type(KEY2, GPIO_INTR_ANYEDGE);
    gpio_set_intr_type(KEY3, GPIO_INTR_ANYEDGE);

    // change gpio intrrupt type for one pin
    gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);

    // create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    // start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    // install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void *)GPIO_INPUT_IO_0);
    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void *)GPIO_INPUT_IO_1);
    // 为GPIO挂起中断服务函数
    gpio_isr_handler_add(KEY1, gpio_isr_handler, (void *)KEY1);
    gpio_isr_handler_add(KEY2, gpio_isr_handler, (void *)KEY2);
    gpio_isr_handler_add(KEY3, gpio_isr_handler, (void *)KEY3);

    // remove isr handler for gpio number.
    gpio_isr_handler_remove(GPIO_INPUT_IO_0);
    // hook isr handler for specific gpio pin again
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void *)GPIO_INPUT_IO_0);

    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

    int cnt = 0;
    while (1)
    {
        printf("cnt: %d\n", cnt++);
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(GPIO_OUTPUT_IO_0, cnt % 2);
        gpio_set_level(GPIO_OUTPUT_IO_1, cnt % 2);
    }
}

// 任务间通过队列发送数据演示代码，运行时需要把前面的代码注释掉
// //队列
// QueueHandle_t xQueue;

// //发送任务
// void Send_Task(void *arg)
// {
//     // 初始化
//     int i = 0;
//     // 循环
//     while (1)
//     {
//         // printf("Send task! i = %d.\n", i++);
//         if (xQueueSend(xQueue, (void *)&i, 0) == pdPASS)
//         {
//             printf("Send!\n");
//         }
//         else
//         {
//             printf("Cannot send!\n");
//         }
//         i++;
//         vTaskDelay(1000 / portTICK_RATE_MS);
//     }
// }

// //接受任务
// void Rce_Task(void *arg)
// {
//     // 初始化
//     int j;
//     // 循环
//     while (1)
//     {
//         if (xQueueReceive(xQueue, (void *)&j, 0) == pdPASS)
//         {
//             printf("Rce ! j = %d.\n", j);
//         }
//         vTaskDelay(1000 / portTICK_RATE_MS);
//     }
// }

// //测试用的主函数
// void app_main(void)
// {

//     xQueue = xQueueCreate(10, sizeof(int));
//     if (xQueue == NULL)
//     {
//         /* The queue could not be created. */
//         printf("The queue cannot be created !\n");
//     }
//     else
//     {
//         xTaskCreate(Send_Task, "Send_Task", 2048, NULL, 1, NULL);
//         xTaskCreate(Rce_Task, "Rce_Task", 2048, NULL, 1, NULL);
//     }
// }