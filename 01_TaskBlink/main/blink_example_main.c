// 2022-07-15 by Fairy
/*
芯片：ESP32 C3
开发环境：VS Code + ESP-IDF
现象：“花式点灯”
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

// 宏定义RGB-LED对应的GPIO口
#define BLINK_GPIO_R 3
#define BLINK_GPIO_G 4
#define BLINK_GPIO_B 5

// LED 结构体
typedef struct
{
    int led_num;    // GPIO
    int delay_time; // 闪烁的间隔时间
    int led_state;  // LED当前的状态
} led_t;

// led闪烁函数的任务
void blink_led_Task(void *pvParam)
{
    // 定义一个LED的结构体变量
    led_t *led_rgb;
    // 强制转换成对应的结构体类型
    led_rgb = (led_t *)pvParam;

    // GPIO初始化
    gpio_reset_pin(led_rgb->led_num);
    // 设置GPIO口的模式为输出模式
    gpio_set_direction(led_rgb->led_num, GPIO_MODE_OUTPUT);

    while (1)
    {
        // 设置GPIO的电平
        gpio_set_level(led_rgb->led_num, led_rgb->led_state);
        // 打印提示信息
        // printf("The GPIO%d is %d.\n", led_rgb->led_num, led_rgb->led_state);
        // 电平翻转
        led_rgb->led_state = !(led_rgb->led_state);
        // 延时一段时间
        vTaskDelay((led_rgb->delay_time) / portTICK_PERIOD_MS);
    }
}

// 定义rgb-led的初始化结构体，作为任务的传递参数
// tips：笔者运行是，结构体需要定义成全局变量才能正常运行
led_t led_rgb_init_r = {
    .led_num = BLINK_GPIO_R,
    .delay_time = 1000,
    .led_state = 1,
};
led_t led_rgb_init_g = {
    .led_num = BLINK_GPIO_G,
    .delay_time = 2000,
    .led_state = 1,
};
led_t led_rgb_init_b = {
    .led_num = BLINK_GPIO_B,
    .delay_time = 3000,
    .led_state = 1,
};

void app_main(void)
{
    // 任务句柄，本例用于得到任务的空闲堆栈空间大小
    TaskHandle_t rgbTaskHandle_r;
    TaskHandle_t rgbTaskHandle_g;
    TaskHandle_t rgbTaskHandle_b;

    // 用于接收Task得到空闲堆栈空间大小的返回值
    UBaseType_t rbgTaski;

    // 创建任务，传入之前定义的结构体参数
    xTaskCreate(blink_led_Task, "LED_R", 2048, (void *)&led_rgb_init_r, 1, &rgbTaskHandle_r);

    xTaskCreate(blink_led_Task, "LED_G", 2048, (void *)&led_rgb_init_g, 1, &rgbTaskHandle_g);

    xTaskCreate(blink_led_Task, "LED_B", 2048, (void *)&led_rgb_init_b, 1, &rgbTaskHandle_b);

    while (1)
    {
        // 得到任务的空闲堆栈大小
        // 返回值越趋近于0，说明剩余空间越小
        // 如果返回值为0或者可能为负，即堆栈溢出，会导致系统重启
        // 不确定的时候可以先设置为一个较大的值，然后调试再适当调整
        rbgTaski = uxTaskGetStackHighWaterMark(rgbTaskHandle_r);
        // 打印得到的堆栈空间的大小
        printf("rbgTaski = %d\t", rbgTaski);

        rbgTaski = uxTaskGetStackHighWaterMark(rgbTaskHandle_g);
        printf("rbgTaski = %d\t", rbgTaski);

        rbgTaski = uxTaskGetStackHighWaterMark(rgbTaskHandle_b);
        printf("rbgTaski = %d\n", rbgTaski);

        vTaskDelay(3000 / portTICK_PERIOD_MS);

        // 在笔者的平台下稳定运行时输出的实例：
        // rbgTaski = 168  rbgTaski = 156  rbgTaski = 160
    }
}
