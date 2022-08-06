/*
 * @Author: Fairy 2754283833@qq.com
 * @Date: 2022-07-22 22:46:21
 * @LastEditTime: 2022-08-04 12:12:11
 * @LastEditors: Fairy
 * @Description: 
 * @FilePath: \single_read\main\single_read.c
 * Copyright (c) 2022 by Fairy 2754283833@qq.com, All Rights Reserved. 
 */
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

// ADC通道
#define ADC1_EXAMPLE_CHAN0 ADC1_CHANNEL_2

static const char *TAG_CH[10] = {"ADC1_CH0"};
static const char *TAG = "ADC SINGLE";

// ADC Attenuation 衰减
#define ADC_EXAMPLE_ATTEN ADC_ATTEN_DB_6

// ADC Calibration 校准
#define ADC_EXAMPLE_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_TP

// ADC原始数据
static int adc_raw;

// 存储ADC特性
static esp_adc_cal_characteristics_t adc1_chars;

// ADC校准初始化
static bool adc_calibration_init(void)
{
    esp_err_t ret;
    bool cali_enable = false;

    // 检查efuse
    ret = esp_adc_cal_check_efuse(ADC_EXAMPLE_CALI_SCHEME);
    if (ret == ESP_ERR_NOT_SUPPORTED)
    {
        // 不支持校准方案，跳过软件校准
        ESP_LOGW(TAG, "Calibration scheme not supported, skip software calibration");
    }
    else if (ret == ESP_ERR_INVALID_VERSION)
    {
        // eFuse没有烧坏，跳过软件校准
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    }
    else if (ret == ESP_OK)
    {
        cali_enable = true;
        // 调用函数初始化存储ADC特性的结构体:adc1_chars
        esp_adc_cal_characterize(ADC_UNIT_1, ADC_EXAMPLE_ATTEN, ADC_WIDTH_BIT_DEFAULT, 0, &adc1_chars);
        printf("\tvref = %d\n", adc1_chars.vref);
        printf("\tatten = %d\n", adc1_chars.atten);
    }
    else
    {
        // 无效
        ESP_LOGE(TAG, "Invalid arg");
    }

    return cali_enable;
}

void app_main(void)
{
    //esp_err_t ret = ESP_OK;
    uint32_t voltage = 0;

    bool cali_enable = adc_calibration_init(); // 校准初始化，成功返回true

    // ADC1 config
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));                         // 配置ADC位宽，手册中是12位
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_EXAMPLE_CHAN0, ADC_EXAMPLE_ATTEN)); // 配置衰减，用于增大测量范围

    while (1)
    {
        // 得到原始数据
        adc_raw = adc1_get_raw(ADC1_EXAMPLE_CHAN0);
        // 监视器打印原始数据
        ESP_LOGI(TAG_CH[0], "raw  data: %d", adc_raw);

        if (cali_enable) // 校准无误
        {
            // 计算电压
            voltage = esp_adc_cal_raw_to_voltage(adc_raw, &adc1_chars);
            // 打印计算值
            ESP_LOGI(TAG_CH[0], "cali data: %d mV", voltage);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
