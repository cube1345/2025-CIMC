#include "ir_cmic_gd32f470vet6.h"

extern config_params_t system_config;
extern storage_state_t storage_state;
extern sampling_state_t sampling_state;

// 系统配置初始化函数
void system_config_init(void)
{
    config_params_t temp_config;
    
    // 尝试从Flash读取配置
    if(read_config_from_flash(&temp_config) == SUCCESS) {
        // 读取成功，更新系统配置
        system_config = temp_config;
        sampling_state.sample_period = system_config.sample_period;
    } else {
        // 读取失败，使用默认值并保存到Flash
        system_config.ratio_ch0 = 1.0f;
        system_config.limit_ch0 = 1.0f;
        system_config.sample_period = 5;
        
        // 保存默认配置到Flash
        write_config_to_flash(&system_config);
    }
}

int main(void)
{
#ifdef __FIRMWARE_VERSION_DEFINE
    uint32_t fw_ver = 0;
#endif
    systick_config();
    init_cycle_counter(false);
    delay_ms(200); // Wait download if SWIO be set to GPIO
    
#ifdef __FIRMWARE_VERSION_DEFINE
    fw_ver = gd32f4xx_firmware_version_get();
    /* print firmware version */
    //printf("\r\nGD32F4xx series firmware version: V%d.%d.%d", (uint8_t)(fw_ver >> 24), (uint8_t)(fw_ver >> 16), (uint8_t)(fw_ver >> 8));
#endif /* __FIRMWARE_VERSION_DEFINE */
    
    bsp_led_init();
    bsp_btn_init();
    bsp_oled_init();
    bsp_gd25qxx_init();
    bsp_usart_init();
    bsp_adc_init();
    bsp_dac_init();
    bsp_rtc_init();
    sd_fatfs_init();
    
    app_btn_init();
    OLED_Init();
    
    // 初始化系统配置和存储系统（包含自动上电计数）
    system_config_init();
    system_startup_init();
	storage_state.log_id = 0;
    scheduler_init();
    while(1) {
        scheduler_run();
     
    
  
    }
}

#ifdef GD_ECLIPSE_GCC
/* retarget the C library printf function to the USART, in Eclipse GCC environment */
int __io_putchar(int ch)
{
    usart_data_transmit(EVAL_COM0, (uint8_t)ch);
    while(RESET == usart_flag_get(EVAL_COM0, USART_FLAG_TBE));
    return ch;
}
#else
/* retarget the C library printf function to the USART */
int fputc(int ch, FILE *f)
{
    usart_data_transmit(USART0, (uint8_t)ch);
    while(RESET == usart_flag_get(USART0, USART_FLAG_TBE));
    return ch;
}
#endif /* GD_ECLIPSE_GCC */
