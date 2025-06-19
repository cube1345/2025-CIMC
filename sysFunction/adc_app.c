#include "ir_cmic_gd32f470vet6.h"

extern uint16_t adc_value[1];
extern uint16_t convertarr[CONVERT_NUM];

extern sampling_state_t sampling_state;
extern config_params_t system_config;

uint32_t last_tick_sampling;
uint32_t last_tick_star;

void sampling_task(void);

void adc_task(void)
{
    convertarr[0] = adc_value[0];
    sampling_task();
}


// 采样任务（需要在主循环中调用）
void sampling_task(void)
{
    if(!sampling_state.is_sampling) {
        return;
    }
    
    uint32_t tick_counter = get_system_ms();
    
    if(tick_counter >= 1000 + last_tick_star)
    {
        last_tick_star = tick_counter;
        LED1_TOGGLE;
    }
    
    if(tick_counter >= (sampling_state.sample_period * 1000) + last_tick_sampling) {
        last_tick_sampling = tick_counter;
        
        // 获取ADC电压值（这里需要实际的ADC读取函数）
        
        float voltage_r = (adc_value[0] * 3.3f / 4095.0f);
        float voltage = voltage_r * system_config.ratio_ch0;
        
        // 检查是否超限
        uint8_t over_limit = (voltage > system_config.limit_ch0) ? 1 : 0;
        
        // 打印采样数据
        print_sample_data(voltage, over_limit);
        
        // 控制LED2
        if(over_limit) {
            // 点亮LED2
            LED2_OFF;
        } else {
            // 熄灭LED2
            LED2_ON;
        }
    }
}

