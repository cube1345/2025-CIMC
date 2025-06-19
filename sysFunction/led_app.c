#include "ir_cmic_gd32f470vet6.h"

uint8_t ucLed[6] = {1,0,1,0,1,0};  // LED ״̬����

/**
 * @brief ��ʾ��ر�Led
 *
 *
 * @param ucLed Led���ݴ�������
 */
void led_disp(uint8_t *ucLed)
{
    // ���ڼ�¼��ǰ LED ״̬����ʱ����
    uint8_t temp = 0x00;
    // ��¼֮ǰ LED ״̬�ı����������ж��Ƿ���Ҫ������ʾ
    static uint8_t temp_old = 0xff;

   for (int i = 0; i < 6; i++)
    {
        // ��LED״̬���ϵ�temp�����У���������Ƚ�
        if (ucLed[i]) temp |= (1<<i); // ����iλ��1
    }

    // ������ǰ״̬��֮ǰ״̬��ͬ��ʱ�򣬲Ÿ�����ʾ
    if (temp_old != temp)
    {
        // ����GPIO��ʼ����������ö�Ӧ����
        LED1_SET(temp & 0x01);
        LED1_SET(temp & 0x02);
        LED1_SET(temp & 0x04);
        LED1_SET(temp & 0x08);
        LED1_SET(temp & 0x10);
        LED1_SET(temp & 0x20);
        
        // ���¾�״̬
        temp_old = temp;
    }
}

/**
 * @brief LED ��ʾ������
 *
 * ÿ�ε��øú���ʱ��LED �Ƹ��� ucLed �����е�ֵ�������ǿ������ǹر�
 */
void led_task(void)
{
    led_disp(ucLed);
}

