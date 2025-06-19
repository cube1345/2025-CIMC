#include "ir_cmic_gd32f470vet6.h"

extern uint16_t adc_value[1];
extern sampling_state_t sampling_state;
extern config_params_t system_config;

/**
 * @brief	使用类似printf的方式显示字符串，显示6x8大小的ASCII字符
 * @param x  Character position on the X-axis  range：0 - 127
 * @param y  Character position on the Y-axis  range：0 - 3
 * 例如：oled_printf(0, 0, "Data = %d", dat);
 **/
int oled_printf(uint8_t x, uint8_t y, const char *format, ...)
{
  char buffer[512]; // 临时存储格式化后的字符串
  va_list arg;      // 处理可变参数
  int len;          // 最终字符串长度

  va_start(arg, format);
  // 安全地格式化字符串到 buffer
  len = vsnprintf(buffer, sizeof(buffer), format, arg);
  va_end(arg);

  OLED_ShowStr(x, y, buffer, 8);
  return len;
}

void oled_task(void)
{
    if(!sampling_state.is_sampling) {
        // 非采样状态：第一行显示"system idle"，第二行为空
        oled_printf(0, 0, "system idle");
    } else {
        // 采样状态：第一行显示时间，第二行显示电压
        uint32_t timestamp = get_unix_timestamp();
        local_time_t local_time = timestamp_to_local_time(timestamp);
        
//        // 格式化时间字符串 (hh:mm:ss)
//        my_printf(0, "%02d:%02d:%02d", local_time.hour, local_time.minute, local_time.second);
//        
        // 获取电压值并格式化
        float voltage_r = (adc_value[0] * 3.3f / 4095.0f);
        float voltage = voltage_r * system_config.ratio_ch0;
        
        int volt_int = (int)voltage;
        int volt_frac = (int)((voltage - volt_int) * 100);
        
        oled_printf(0, 0, "%02d:%02d:%02d   ", local_time.hour, local_time.minute, local_time.second);
        oled_printf(0, 1, "%d.%02d V   ", volt_int, volt_frac);
    }
}

/* CUSTOM EDIT */
