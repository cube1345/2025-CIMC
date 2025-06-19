#ifndef __USART_APP_H__
#define __USART_APP_H__

#include "stdint.h"
#include "gd32f4xx.h"
#include "sd_app.h"  // 包含存储相关函数声明
#include "rtc_app.h"

#ifdef __cplusplus
extern "C" {
#endif

// 配置管理函数声明
typedef struct {
    float ratio_ch0;
    float limit_ch0;
    uint32_t sample_period; // 采样周期，单位秒
} config_params_t;

// 采样状态结构体
typedef struct {
    uint8_t is_sampling;      // 采样状态：0=停止，1=运行
    uint32_t sample_period;   // 采样周期，单位秒
    uint32_t last_sample_time; // 上次采样时间（系统tick）
    uint8_t hide_mode;        // 数据隐藏模式：0=正常显示，1=隐藏显示
} sampling_state_t;

int my_printf(uint32_t usart_periph, const char *format, ...);
void uart_task(void);


// 命令处理函数声明
void process_command(char* command);
void system_self_test(void);
void rtc_config_handler(char* time_str);
void rtc_show_current_time(void);
void config_read_handler(void);

// 新增的参数设置函数
void ratio_setting_handler(void);
void limit_setting_handler(void);
void config_save_handler(void);
void config_read_flash_handler(void);

// 配置文件操作函数
void save_config_to_file(void);

// 浮点数打印辅助函数
void print_float_manual(uint32_t usart_periph, const char* prefix, float value, const char* suffix);

// 输入验证函数
int is_valid_number(const char* str);

// 采样控制相关函数
void sampling_start_handler(void);
void sampling_stop_handler(void);
void hide_data_handler(void);
void unhide_data_handler(void);

// RTC调试函数
void rtc_debug_handler(void);

// 系统初始化函数
void system_config_init(void);

// Flash配置读写函数
ErrStatus write_config_to_flash(config_params_t* config);
ErrStatus read_config_from_flash(config_params_t* config);

// 采样数据输出和时间戳函数
void print_sample_data(float voltage, uint8_t over_limit);
uint32_t get_unix_timestamp(void);  // 使用库函数获取Unix时间戳

// 时区转换函数
local_time_t timestamp_to_local_time(uint32_t timestamp);

// 数据存储相关结构体
typedef struct {
    uint8_t sample_count;           // 当前sample文件中的数据条数
    uint8_t overlimit_count;        // 当前overlimit文件中的数据条数
    uint8_t hidedata_count;         // 当前hidedata文件中的数据条数
    uint32_t log_id;                // 日志文件ID（上电次数）
    uint8_t hide_storage_enabled;   // 是否启用加密存储
} storage_state_t;

// 系统初始化函数
void system_config_init(void);

// 配置保存函数（实际实现在sd_app.c中）
void save_config_to_file(void);

// 系统启动初始化相关函数
void system_startup_init(void);
void read_device_id_from_flash(char* device_id, uint16_t max_len);
void write_device_id_to_flash(const char* device_id);
void device_id_setting_handler(char* new_id);
void rtc_debug_info(void);
void power_count_handler(void);
void power_count_reset_handler(void);

#ifdef __cplusplus
}
#endif

#endif
