#include <stdio.h>
#include "OLED.h"

/**
 * @brief i2c master initialization
 */
esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO, // 18
        .scl_io_num = I2C_MASTER_SCL_IO, // 19
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

/**
 * @description: OLED 发送一个字节
 * @return       错误信息
 * @param {uint8_t} data 需要发送的内容，数据或者命令
 * @param {uint8_t} cmd_ 1:发送数据 0:发送命令
 */
esp_err_t OLED_WR_Byte(uint8_t data, uint8_t cmd_)
{
    int ret;

    uint8_t write_buf[2] = {((cmd_ == 1) ? (0x40) : (0x00)), data};

    ret = i2c_master_write_to_device(I2C_MASTER_NUM, OLED_ADDR, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS);

    return ret;
}

/**
 * @description: OLED 屏幕初始化
 * @return       无
 */
void OLED_Init(void)
{
    OLED_WR_Byte(0xAE, OLED_CMD); //--display off
    OLED_WR_Byte(0x00, OLED_CMD); //---set low column address
    OLED_WR_Byte(0x10, OLED_CMD); //---set high column address
    OLED_WR_Byte(0x40, OLED_CMD); //--set start line address
    OLED_WR_Byte(0xB0, OLED_CMD); //--set page address
    OLED_WR_Byte(0x81, OLED_CMD); // contract control
    OLED_WR_Byte(0xFF, OLED_CMD); //--128
    OLED_WR_Byte(0xA1, OLED_CMD); // set segment remap
    OLED_WR_Byte(0xA6, OLED_CMD); //--normal / reverse
    OLED_WR_Byte(0xA8, OLED_CMD); //--set multiplex ratio(1 to 64)
    OLED_WR_Byte(0x3F, OLED_CMD); //--1/32 duty
    OLED_WR_Byte(0xC8, OLED_CMD); // Com scan direction
    OLED_WR_Byte(0xD3, OLED_CMD); //-set display offset
    OLED_WR_Byte(0x00, OLED_CMD); //
    OLED_WR_Byte(0xD5, OLED_CMD); // set osc division
    OLED_WR_Byte(0x80, OLED_CMD); //
    OLED_WR_Byte(0xD8, OLED_CMD); // set area color mode off
    OLED_WR_Byte(0x05, OLED_CMD); //
    OLED_WR_Byte(0xD9, OLED_CMD); // Set Pre-Charge Period
    OLED_WR_Byte(0xF1, OLED_CMD); //
    OLED_WR_Byte(0xDA, OLED_CMD); // set com pin configuartion
    OLED_WR_Byte(0x12, OLED_CMD); //
    OLED_WR_Byte(0xDB, OLED_CMD); // set Vcomh
    OLED_WR_Byte(0x30, OLED_CMD); //
    OLED_WR_Byte(0x8D, OLED_CMD); // set charge pump enable
    OLED_WR_Byte(0x14, OLED_CMD); //
    OLED_WR_Byte(0xAF, OLED_CMD); //--turn on oled panel
    OLED_Clear();
}

/**
 * @description: OLED 屏幕 设置坐标
 * @return       无
 * @param {uint8_t} x 坐标x轴，范围0~127
 * @param {uint8_t} y 坐标y轴，范围0~63
 */
void OLED_Set_Pos(uint8_t x, uint8_t y)
{
    OLED_WR_Byte(0xb0 + y, OLED_CMD);
    OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, OLED_CMD);
    OLED_WR_Byte((x & 0x0f), OLED_CMD);
}

/**
 * @description: OLED 清屏
 * @return       无
 */
void OLED_Clear(void)
{
    uint8_t i, n;
    for (i = 0; i < 8; i++)
    {
        OLED_WR_Byte(0xb0 + i, OLED_CMD);
        OLED_WR_Byte(0x00, OLED_CMD);
        OLED_WR_Byte(0x10, OLED_CMD);
        for (n = 0; n < 128; n++)
            OLED_WR_Byte(0, OLED_DATA);
    }
}

/**
 * @description: OLED 显示单个字符
 * @return       无
 * @param {uint8_t} x 显示字符的x坐标，范围0~127
 * @param {uint8_t} y 显示字符的y坐标，字符大小为16，取值0,2,4,6；字符大小6，取值0,1,2,3,4,5,6,7
 * @param {uint8_t} chr 显示的单个字符，在字库中出现的字符
 * @param {uint8_t} Char_Size 字符大小，取16或者8
 */
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t Char_Size)
{
    uint8_t c = 0;
    uint8_t i = 0;
    c = chr - ' ';
    if (x > 127)
    {
        x = 0;
        y = y + 2;
    }
    if (Char_Size == 16)
    {
        OLED_Set_Pos(x, y);
        for (i = 0; i < 8; i++)
            OLED_WR_Byte(F8X16[c * 16 + i], OLED_DATA);
        OLED_Set_Pos(x, y + 1);
        for (i = 0; i < 8; i++)
            OLED_WR_Byte(F8X16[c * 16 + i + 8], OLED_DATA);
    }
    else
    {
        OLED_Set_Pos(x, y);
        for (i = 0; i < 6; i++)
            OLED_WR_Byte(F6x8[c][i], OLED_DATA);
    }
}

/**
 * @description: OLED 显示字符串，会自动换行
 * @return       无
 * @param {uint8_t} x 显示字符串第一个字符的x坐标，范围0~127
 * @param {uint8_t} y 显示字符串第一个字符的y坐标，字符大小为16，取值0,2,4,6；字符大小6，取值0,1,2,3,4,5,6,7
 * @param {char} *chr 显示的字符串
 * @param {uint8_t} Char_Size 字符大小，取16或者8
 */
void OLED_ShowString(uint8_t x, uint8_t y, char *chr, uint8_t Char_Size)
{
    unsigned char j = 0;
    while (chr[j] != '\0')
    {
        OLED_ShowChar(x, y, chr[j], Char_Size);
        x += 8;
        if (x > 120)
        {
            x = 0;
            y += 2;
        }
        j++;
    }
}

/**
 * @description: OLED 显示汉字
 * @return       无
 * @param {uint8_t} x 显示汉字的x坐标
 * @param {uint8_t} y 显示汉字的y坐标
 * @param {uint8_t} no 显示汉字在字库中的序号
 */
void OLED_ShowCHinese(uint8_t x, uint8_t y, uint8_t no)
{
    uint8_t t, adder = 0;
    OLED_Set_Pos(x, y);
    for (t = 0; t < 16; t++)
    {
        OLED_WR_Byte(Hzk[2 * no][t], OLED_DATA);
        adder += 1;
    }
    OLED_Set_Pos(x, y + 1);
    for (t = 0; t < 16; t++)
    {
        OLED_WR_Byte(Hzk[2 * no + 1][t], OLED_DATA);
        adder += 1;
    }
}

/**
 * @description: 求m^n的函数
 * @return       m^n的结果
 * @param {uint8_t} m 底数
 * @param {uint8_t} n 指数
 */
uint32_t oled_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;
    while (n--)
        result *= m;
    return result;
}

/**
 * @description: OLED 显示数字
 * @return       无
 * @param {uint8_t} x 显示数字的第一个位置的x坐标
 * @param {uint8_t} y 显示数字的第一个位置的y坐标
 * @param {uint32_t} num 欲显示的数字
 * @param {uint8_t} len 显示所占的长度，不建议小于真正要显示的数字的长度
 * @param {uint8_t} size2 显示的数字的大小，16、8可选
 */
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size2)
{
    uint8_t t, temp;
    uint8_t enshow = 0;
    for (t = 0; t < len; t++)
    {
        temp = (num / oled_pow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0)
            {
                OLED_ShowChar(x + (size2 / 2) * t, y, ' ', size2);
                continue;
            }
            else
                enshow = 1;
        }
        OLED_ShowChar(x + (size2 / 2) * t, y, temp + '0', size2);
    }
}