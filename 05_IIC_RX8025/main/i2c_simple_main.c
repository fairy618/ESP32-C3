/* i2c - Simple example

   Simple I2C example that shows how to initialize I2C
   as well as reading and writing from and to registers for a sensor connected over I2C.

   The sensor used in this example is a MPU9250 inertial measurement unit.

   For other examples please check:
   https://github.com/espressif/esp-idf/tree/master/examples

   See README.md file to get detailed usage of this example.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"

static const char *TAG = "i2c-simple-example";

#define I2C_MASTER_SCL_IO 17        /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO 18        /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM 0            /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ 400000   /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS 1000

// 重要！
#define RX8025_SENSOR_ADDR 0x32

unsigned char Rx8025SetData[7] = {0x22, 0x08, 0x09, 0x02, 0x12, 0x59, 0x30};
//                                 年    月     日  星期    时     分    秒
unsigned char Rx8025ReadData[7];

static esp_err_t i2c_master_init(void);
static esp_err_t rx8025_register_write_byte(uint8_t reg_addr, uint8_t data);
static esp_err_t rx8025_register_read(uint8_t reg_addr, uint8_t *data, size_t len);
void RX8025_Init(void);
void rx8025_get_time(void);
void rx8025_set_time(void);
// unsigned char Rx8025_Check_Status(void);

void app_main(void)
{
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    // 初始化RX8025
    RX8025_Init();

    // 根据 Rx8025SetData 数组的内容设置RTC时间
    rx8025_set_time();

    while (1)
    {
        // 读取时间
        rx8025_get_time();

        // 打印
        printf("Data:20%d-%x-%x\tTime:%x:%x:%x\n",
               Rx8025ReadData[0],
               Rx8025ReadData[1],
               Rx8025ReadData[2],
               Rx8025ReadData[4],
               Rx8025ReadData[5],
               Rx8025ReadData[6]);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/**
 * @description:
 * @return      {*}
 * @param {uint8_t} reg_addr
 * @param {uint8_t} *data
 * @param {size_t} len
 */
static esp_err_t rx8025_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, RX8025_SENSOR_ADDR, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
}

/**
 * @description:
 * @return      {*}
 * @param {uint8_t} reg_addr
 * @param {uint8_t} data
 */
static esp_err_t rx8025_register_write_byte(uint8_t reg_addr, uint8_t data)
{
    int ret;
    uint8_t write_buf[2] = {reg_addr, data};

    ret = i2c_master_write_to_device(I2C_MASTER_NUM, RX8025_SENSOR_ADDR, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);

    return ret;
}
/**
 * @description:
 * @return      {*}
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

void RX8025_Init(void)
{
    /*
            地址：0x0F  名称：控制寄存器
    0    位7 CSEL1 ：温度补偿间隔，默认0
    1    位6 CSEL0 ：温度补偿间隔，默认1【2s】
    0    位5  UIE  ：[Update Interrupt Enable] 时间更新中断引脚，置一中断生效
    0    位4  TLE  ：[Timer Interrupt Enable]  固定周期计时器中断，置一中断生效

    0    位3  AIE  ：[Alarm Interrupt Enable]  闹钟计时器中断，置一中断生效
    0    位2 'ZERO ：写保护位，读出值位0
    0    位1 'ZERO ：写保护位，读出值位0
    0    位0 RESET ：写个0进去吧，写1好像时要停止啥
    */
    rx8025_register_write_byte(0x0F, 0x40);
    /*
                地址：0x0E  名称：标志位寄存器
    0    位7 'ZERO ：写保护位，读出值位0
    0    位6 'ZERO ：写保护位，读出值位0
    0    位5   UF  ：[Update Flag] time update interrupt 需先复位为0，需手动清零
    0    位4   TF  ：[Timer Flag]  fixed-cycle timer interrupt 需先复位为0，需手动清零

    0    位3   AF  ：[ Alarm Flag] alarm interrupt 需先复位为0，需手动清零
    0    位2 'ZERO ：写保护位，读出值位0
    0    位1  VLF  ：[Voltage Low Flag] 检测电压导致的数据丢失。预先写0，数据丢失会置一，需手动清零
    0    位0  VDET ：[Voltage Detection Flag] 温度补偿标志位。读出来是0就正常；预先写0，读出来1说明温度补偿异常
    */
    rx8025_register_write_byte(0x0E, 0x00);
    /*
                地址：0x0D  名称：扩展寄存器
    0    位7  TEST ：[0 Normal operation mode]
    1    位6  WADA ：[Week Alarm/Day Alarm] 1:DAY/0:WEEK 设置闹钟的比较对象，日还是周
    0    位5  USEL ：[Update Interrupt Select] 原文update generation timing of the time update interrupt function ，默认值0
    0    位4   TE  ：[Timer Enable] 倒计时（fixed-cycle timer）计时器使能标志位，1开始，0停止

    0    位3 FSEL1 ：[FOUT frequency Select 0, 1]
    0    位2 FSEL0 ：[FOUT frequency Select 0, 1]
    1    位1 TSEL1 ：[Timer Select 0, 1]    用于选择 fixed-cycle timer的周期
    0    位0 TSEL0 ：[Timer Select 0, 1]    详见手册P14  8.2.4  6）
    */
    rx8025_register_write_byte(0x0D, 0x42);
}

/***************************************************
 *@description  : RX8025 将数组的数据写入RX8025
 *@param         {*}
 *@return        {*}
 ***************************************************/
void rx8025_set_time(void)
{
    // rx8025_register_write_byte(0x06, 22); // 年
    rx8025_register_write_byte(0x05, Rx8025SetData[1]); // 月
    rx8025_register_write_byte(0x04, Rx8025SetData[2]); // 日
    if (Rx8025SetData[3] == 0x07)                       // Sunday，对应0x01
    {
        rx8025_register_write_byte(0x03, 0x01); // 星期（7）
    }
    else
    {
        rx8025_register_write_byte(0x03, 0x01 << Rx8025SetData[3]); // 星期（1~6）
    }
    rx8025_register_write_byte(0x02, Rx8025SetData[4]); // 时
    rx8025_register_write_byte(0x01, Rx8025SetData[5]); // 分
    rx8025_register_write_byte(0x00, Rx8025SetData[6]); // 秒
}

/***************************************************
 *@description  : RX8025 读寄存器数据获取时间
 *@param         {*}
 *@return        {*}
 ***************************************************/
void rx8025_get_time(void)
{
    rx8025_register_read(0x06, &Rx8025ReadData[0], 1);
    rx8025_register_read(0x05, &Rx8025ReadData[1], 1);
    rx8025_register_read(0x04, &Rx8025ReadData[2], 1);
    rx8025_register_read(0x03, &Rx8025ReadData[3], 1);
    rx8025_register_read(0x02, &Rx8025ReadData[4], 1);
    rx8025_register_read(0x01, &Rx8025ReadData[5], 1);
    rx8025_register_read(0x00, &Rx8025ReadData[6], 1);

    // rx8025_register_read(0x06, year, 1);
    // printf("year = %d\n", year[0]);
}

/***************************************************
*@description  : RX8025 检测芯片状态，检测到异常蜂鸣器报警
*@param         {*}
*@return        {unsigned char} Status  芯片状态
                                        0x00 正常
                                        0x01 温度补偿不工作
                                        0x02 数据丢失
                                        0x03 温度补偿不工作 & 数据丢失
***************************************************/
// unsigned char Rx8025_Check_Status(void)
// {
//     unsigned char FlagReg; // 标志寄存器
//     unsigned char Status = 0x00;
//     rx8025_register_read(0x0E ,&FlagReg,1); // 读寄存器

//     if (FlagReg & 0x01 == 0x01)
//     {
//         /*
//         bit 0 : VDET [Voltage Detection Flag]
//         如若检测出该位未高，温度补偿将会停止检测
//         只能写0，写1无效！
//         */
//         // beep = 0;
//         Status |= 0x01;
//     }
//     if (FlagReg & 0x02 == 0x02)
//     {
//         /*
//         bit 1 : VLF [Voltage Low Flag]
//         如若检测到1，说明芯片检测到数据丢失，所有的寄存器都需要重新赋值
//             检测到0，说明芯片未检测到数据丢失
//         写入1之后该位无效，写0即准备下一次检测
//         */
//         // beep = 0;
//         Status |= 0x02;
//     }
//     return Status;
// }