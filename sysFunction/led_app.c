#include "ir_cmic_gd32f470vet6.h"

uint8_t ucLed[6] = {1,0,1,0,1,0};  // LED 状态数组

/**
 * @brief 显示或关闭Led
 *
 *
 * @param ucLed Led数据储存数组
 */
void led_disp(uint8_t *ucLed)
{
    // 用于记录当前 LED 状态的临时变量
    uint8_t temp = 0x00;
    // 记录之前 LED 状态的变量，用于判断是否需要更新显示
    static uint8_t temp_old = 0xff;

   for (int i = 0; i < 6; i++)
    {
        // 将LED状态整合到temp变量中，方便后续比较
        if (ucLed[i]) temp |= (1<<i); // 将第i位置1
    }

    // 仅当当前状态与之前状态不同的时候，才更新显示
    if (temp_old != temp)
    {
        // 根据GPIO初始化情况，设置对应引脚
        LED1_SET(temp & 0x01);
        LED1_SET(temp & 0x02);
        LED1_SET(temp & 0x04);
        LED1_SET(temp & 0x08);
        LED1_SET(temp & 0x10);
        LED1_SET(temp & 0x20);
        
        // 更新旧状态
        temp_old = temp;
    }
}

/**
 * @brief LED 显示处理函数
 *
 * 每次调用该函数时，LED 灯根据 ucLed 数组中的值来决定是开启还是关闭
 */
void led_task(void)
{
    led_disp(ucLed);
}

