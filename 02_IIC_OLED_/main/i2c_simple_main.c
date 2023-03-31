/*
 * @Author: Fairy 2754283833@qq.com
 * @Date: 2022-07-22 22:58:10
 * @LastEditTime: 2022-08-04 12:07:54
 * @LastEditors: Fairy
 * @Description:
 * @FilePath: \i2c_simple+\main\i2c_simple_main.c
 * Copyright (c) 2022 by Fairy 2754283833@qq.com, All Rights Reserved.
 */

#include "OLED.h"

static const char *TAG = "i2c-simple-example";

/**
 * @description: 主函数
 * @return       无
 */
void app_main(void)
{
    // IIC总线主机初始化
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    // OLED屏幕初始化
    OLED_Init();

    // 显示汉字
    OLED_ShowCHinese(0 * 18, 0, 0);
    OLED_ShowCHinese(1 * 18, 0, 1);
    OLED_ShowCHinese(2 * 18, 0, 2);

    // 显示单个字符
    OLED_ShowChar(0, 2, 'Q', 16);

    // 显示字符串
    OLED_ShowString(0, 4, "Fairy tale", 16);

    // 显示数字
    OLED_ShowNum(0, 6, 8266, 6, 16);

    // 删除IIC设备
    // ESP_ERROR_CHECK(i2c_driver_delete(I2C_MASTER_NUM));
    // ESP_LOGI(TAG, "I2C unitialized successfully");
}
