#include "ir_cmic_gd32f470vet6.h"

extern uint16_t adc_value[1];
extern sampling_state_t sampling_state;
extern config_params_t system_config;

/**
 * @brief	ʹ������printf�ķ�ʽ��ʾ�ַ�������ʾ6x8��С��ASCII�ַ�
 * @param x  Character position on the X-axis  range��0 - 127
 * @param y  Character position on the Y-axis  range��0 - 3
 * ���磺oled_printf(0, 0, "Data = %d", dat);
 **/
int oled_printf(uint8_t x, uint8_t y, const char *format, ...)
{
  char buffer[512]; // ��ʱ�洢��ʽ������ַ���
  va_list arg;      // ����ɱ����
  int len;          // �����ַ�������

  va_start(arg, format);
  // ��ȫ�ظ�ʽ���ַ����� buffer
  len = vsnprintf(buffer, sizeof(buffer), format, arg);
  va_end(arg);

  OLED_ShowStr(x, y, buffer, 8);
  return len;
}

void oled_task(void)
{
    if(!sampling_state.is_sampling) {
        // �ǲ���״̬����һ����ʾ"system idle"���ڶ���Ϊ��
        oled_printf(0, 0, "system idle");
    } else {
        // ����״̬����һ����ʾʱ�䣬�ڶ�����ʾ��ѹ
        uint32_t timestamp = get_unix_timestamp();
        local_time_t local_time = timestamp_to_local_time(timestamp);
        
//        // ��ʽ��ʱ���ַ��� (hh:mm:ss)
//        my_printf(0, "%02d:%02d:%02d", local_time.hour, local_time.minute, local_time.second);
//        
        // ��ȡ��ѹֵ����ʽ��
        float voltage_r = (adc_value[0] * 3.3f / 4095.0f);
        float voltage = voltage_r * system_config.ratio_ch0;
        
        int volt_int = (int)voltage;
        int volt_frac = (int)((voltage - volt_int) * 100);
        
        oled_printf(0, 0, "%02d:%02d:%02d   ", local_time.hour, local_time.minute, local_time.second);
        oled_printf(0, 1, "%d.%02d V   ", volt_int, volt_frac);
    }
}

/* CUSTOM EDIT */
