
#include "ir_cmic_gd32f470vet6.h"

/* =================== 全局变量定义 =================== */

__IO uint16_t tx_count = 0;
__IO uint8_t rx_flag = 0;
uint8_t uart_dma_buffer[512] = {0};
extern __IO uint32_t prescaler_a, prescaler_s;
extern uint16_t adc_value[1];

// 全局配置参数
config_params_t system_config = {1.0f, 1.0f, 5}; // 默认值：ratio=1.0, limit=1.0, period=5s

// 采样状态
sampling_state_t sampling_state = {0, 5, 0, 0}; 

storage_state_t storage_state = {0, 0, 0, 0, 0};

static volatile uint8_t waiting_for_ratio_input = 0;
static volatile uint8_t waiting_for_limit_input = 0;
static volatile uint8_t waiting_for_rtc_input = 0;

/* =================== USART通信模块 =================== */

/**
 * @brief 格式化输出到指定USART端口
 *
 * 类似printf的功能，支持格式化字符串输出到指定的USART端口。
 * 内部使用512字节缓冲区，支持可变参数列表。采用轮询方式发送数据，
 * 确保每个字符都成功发送后再发送下一个字符。
 *
 * @param usart_periph USART外设编号（如USART0、USART1等），必须是有效的USART外设
 * @param format 格式化字符串，支持标准printf格式说明符（%d、%s、%f等）
 * @param ... 可变参数列表，对应格式化字符串中的占位符
 * @return 实际发送的字符数量，如果格式化失败则返回负值
 * @note 最大支持512字符的输出，超出部分将被截断
 * @warning 确保USART端口已正确初始化后再调用此函数，发送过程中会阻塞等待
 */
int my_printf(uint32_t usart_periph, const char *format, ...)
{
    char buffer[512];
    va_list arg;
    int len;

    va_start(arg, format);
    len = vsnprintf(buffer, sizeof(buffer), format, arg);
    va_end(arg);
    
    for(tx_count = 0; tx_count < len; tx_count++){
        usart_data_transmit(usart_periph, buffer[tx_count]);
        while(RESET == usart_flag_get(usart_periph, USART_FLAG_TBE));
    }
    
    return len;
}

/**
 * @brief UART任务处理函数
 *
 * 检查UART接收标志，处理通过DMA接收到的命令数据。
 * 自动去除命令字符串末尾的换行符、回车符和空格，
 * 然后调用命令处理函数进行解析和执行。处理完成后清空接收缓冲区。
 *
 * @return 无返回值
 * @note 此函数应在主循环中周期性调用，依赖全局变量rx_flag和uart_dma_buffer
 * @warning 确保UART DMA接收已正确配置，且uart_dma_buffer大小足够
 */
void uart_task(void)
{
    if(!rx_flag) return;
    

    char* command = (char*)uart_dma_buffer;
    

    int len = strlen(command);
    while(len > 0 && (command[len-1] == '\r' || command[len-1] == '\n' || command[len-1] == ' ')) {
        command[len-1] = '\0';
        len--;
    }
    
    if(len > 0) {
        process_command(command);
    }
    
    memset(uart_dma_buffer, 0, 256);
    rx_flag = 0;
}

/* =================== 命令处理模块 =================== */

/**
 * @brief 处理接收到的串口命令
 * @param command 接收到的命令字符串，已去除换行符和空格
 * @return 无返回值
 * @note 支持交互式参数输入和多种系统命令，包括测试、配置、采样控制等
 */
void process_command(char* command)
{
    if(waiting_for_ratio_input) {
        float input_value = atof(command);
        my_printf(DEBUG_USART, "%.1f\r\n", input_value);
        if(input_value >= 0.0f && input_value <= 100.0f) {
            system_config.ratio_ch0 = input_value;
            
            my_printf(DEBUG_USART, "ratio modified success\r\n");
            // print_float_manual(DEBUG_USART, "Ratio = ", system_config.ratio_ch0, "\r\n");
            my_printf(DEBUG_USART, "Ratio = %.1f\r\n", system_config.ratio_ch0);
            
            char log_msg[64];
            sprintf(log_msg, "ratio config success to %.1f", input_value);
            log_operation(log_msg);
            

            save_config_to_file();
        } else {
            my_printf(DEBUG_USART, "ratio invalid\r\n");
            my_printf(DEBUG_USART, "Ratio = %.1f\r\n", system_config.ratio_ch0);
        }
        waiting_for_ratio_input = 0;
        return;
    }
    if(waiting_for_limit_input) {
        float input_value = atof(command);
        my_printf(DEBUG_USART, "%.2f\r\n", input_value);
        if(input_value >= 0.0f && input_value <= 200.0f) {
            system_config.limit_ch0 = input_value;

            my_printf(DEBUG_USART, "limit modified success\r\n");
            // print_float_manual(DEBUG_USART, "limit = ", system_config.limit_ch0, "\r\n");
            my_printf(DEBUG_USART, "limit = %.2f\r\n", system_config.limit_ch0);
            
            char log_msg[64];
            sprintf(log_msg, "limit config success to %.2f", input_value);
            log_operation(log_msg);
            
            save_config_to_file();
        } else {
            my_printf(DEBUG_USART, "limit invalid\r\n");
            // print_float_manual(DEBUG_USART, "limit = ", system_config.limit_ch0, "\r\n");
            my_printf(DEBUG_USART, "limit = %.2f\r\n", system_config.limit_ch0);
        }
        waiting_for_limit_input = 0;
        return;
    }

    // 处理RTC时间输入
    if(waiting_for_rtc_input) {
        log_operation("rtc config");
        rtc_config_handler(command);
        waiting_for_rtc_input = 0;
        return;
    }

    my_printf(DEBUG_USART, "%s\r\n", command);

    if(strcmp(command, "test") == 0) {
        system_self_test();
    }
    else if(strcmp(command, "RTC Config") == 0) {
        // 2.1 串口返回"Input Datetime"
        my_printf(DEBUG_USART, "Input Datetime\r\n");
        waiting_for_rtc_input = 1;
    }
    else if(strcmp(command, "RTC now") == 0) {
        rtc_show_current_time();
    }
    else if(strcmp(command, "conf") == 0) {
        config_read_handler();
    }
    else if(strcmp(command, "ratio") == 0) {
        ratio_setting_handler();
    }
    else if(strcmp(command, "limit") == 0) {
        limit_setting_handler();
    }
    else if(strcmp(command, "config save") == 0) {
        config_save_handler();
    }
    else if(strcmp(command, "config read") == 0) {
        config_read_flash_handler();
    }
    else if(strcmp(command, "start") == 0) {
        sampling_start_handler();
    }
    else if(strcmp(command, "stop") == 0) {
        sampling_stop_handler();
    }
    else if(strcmp(command, "hide") == 0) {
        hide_data_handler();
    }
    else if(strcmp(command, "unhide") == 0) {
        unhide_data_handler();
    }
    else if(strcmp(command, "storage status") == 0) {
        my_printf(DEBUG_USART, "=== Storage Status ===\r\n");
        my_printf(DEBUG_USART, "Sample count: %d/10\r\n", storage_state.sample_count);
        my_printf(DEBUG_USART, "OverLimit count: %d/10\r\n", storage_state.overlimit_count);
        my_printf(DEBUG_USART, "HideData count: %d/10\r\n", storage_state.hidedata_count);
        my_printf(DEBUG_USART, "Log ID: %u\r\n", storage_state.log_id);
        my_printf(DEBUG_USART, "Hide storage: %s\r\n", storage_state.hide_storage_enabled ? "Enabled" : "Disabled");
        my_printf(DEBUG_USART, "=====================\r\n");
    }
    // 设备ID设置命令
    else if(strncmp(command, "device id", 9) == 0) {
        char* id_param = command + 9;
        while(*id_param == ' ') id_param++; // 跳过空格
        if(*id_param != '\0') {
            device_id_setting_handler(id_param);
        } else {
            // 显示当前设备ID
            char current_id[32];
            read_device_id_from_flash(current_id, sizeof(current_id));
            my_printf(DEBUG_USART, "Current Device ID: %s\r\n", current_id);
            my_printf(DEBUG_USART, "Usage: device id 2025-CIMC-XXX\r\n");
        }
    }
    // RTC调试命令
    else if(strcmp(command, "rtc debug") == 0) {
        rtc_debug_info();
    }
    // 上电次数查询命令
    else if(strcmp(command, "power count") == 0) {
        power_count_handler();
    }
    // 上电次数重置命令
    else if(strcmp(command, "power reset") == 0) {
        power_count_reset_handler();
    }
    else {
        my_printf(DEBUG_USART, "Unknown command: %s\r\n", command);
        my_printf(DEBUG_USART, "Available commands:\r\n");
        my_printf(DEBUG_USART, "  test - System self test\r\n");
        my_printf(DEBUG_USART, "  RTC Config - Set RTC time (interactive mode)\r\n");
        my_printf(DEBUG_USART, "  RTC now - Show current time (UTC+8)\r\n");
        my_printf(DEBUG_USART, "  conf - Read config from TF card\r\n");
        my_printf(DEBUG_USART, "  ratio - Set ratio parameter\r\n");
        my_printf(DEBUG_USART, "  limit - Set limit parameter\r\n");
        my_printf(DEBUG_USART, "  config save - Save parameters to flash\r\n");
        my_printf(DEBUG_USART, "  config read - Read parameters from flash\r\n");
        my_printf(DEBUG_USART, "  start - Start periodic sampling\r\n");
        my_printf(DEBUG_USART, "  stop - Stop periodic sampling\r\n");
        my_printf(DEBUG_USART, "  hide - Hide data format\r\n");
        my_printf(DEBUG_USART, "  unhide - Unhide data format\r\n");
        my_printf(DEBUG_USART, "  storage status - Show storage status\r\n");
        my_printf(DEBUG_USART, "  device id [ID] - Set/Show device ID (Format: 2025-CIMC-XXX)\r\n");
        my_printf(DEBUG_USART, "  rtc debug - Show RTC debug information\r\n");
        my_printf(DEBUG_USART, "  power count - Show power-on count and system info\r\n");
        my_printf(DEBUG_USART, "  power reset - Reset power-on count to 0\r\n");
    }
}

/* =================== 系统测试模块 =================== */

/**
 * @brief 执行系统硬件自检测试
 * @return 无返回值
 * @note 检测Flash、TF卡、RTC状态并输出测试结果，记录测试日志
 */
void system_self_test(void)
{
    log_operation("system hardware test");

    my_printf(DEBUG_USART, "======System selftest======\r\n");

    uint32_t flash_id = spi_flash_read_id();
    if(flash_id != 0x000000 && flash_id != 0xFFFFFF) {
        my_printf(DEBUG_USART, "flash......ok\r\n");
    } else {
        my_printf(DEBUG_USART, "flash......error\r\n");
    }

    DSTATUS sd_status = disk_initialize(0);
    if(sd_status == 0) {
        my_printf(DEBUG_USART, "TF card......ok\r\n");
        my_printf(DEBUG_USART, "flash ID:0x%06X\r\n", flash_id);
        uint32_t capacity = sd_card_capacity_get();
        my_printf(DEBUG_USART, "TF card memory: %d KB\r\n", capacity);
        log_operation("test ok");
    } else {
        my_printf(DEBUG_USART, "TF card.......error\r\n");
        my_printf(DEBUG_USART, "flash ID:0x%06X\r\n", flash_id);
        my_printf(DEBUG_USART, "can not find TF card\r\n");
        log_operation("test error: tf card not found");
    }
    uint32_t timestamp = get_unix_timestamp();
    local_time_t local_time = timestamp_to_local_time(timestamp);
    my_printf(DEBUG_USART, "RTC: %04d-%02d-%02d %02d:%02d:%02d\r\n",
              local_time.year, local_time.month, local_time.day,
              local_time.hour, local_time.minute, local_time.second);

    my_printf(DEBUG_USART, "=====System selftest=====\r\n\r\n");
}

/* =================== RTC配置模块 =================== */

/**
 * @brief 处理RTC时间配置命令
 * @param time_str 时间字符串，支持多种格式（YYYY-MM-DD HH:MM:SS等）
 * @return 无返回值
 * @note 自动转换东八区时间为UTC时间存储，支持跨日期处理
 */
void rtc_config_handler(char* time_str)
{
    int year, month, day, hour, minute, second;

    int parsed = 0;

    // 2.2 支持标准时间格式："2025-01-01 15:00:10"
    parsed = sscanf(time_str, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);

    // 备用格式1：空格分隔
    if(parsed != 6) {
        parsed = sscanf(time_str, "%d %d %d %d %d %d", &year, &month, &day, &hour, &minute, &second);
    }

    // 备用格式2：斜杠分隔
    if(parsed != 6) {
        parsed = sscanf(time_str, "%d/%d/%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
    }

    // 备用格式3：横杠分隔时间
    if(parsed != 6) {
        parsed = sscanf(time_str, "%d-%d-%d %d-%d-%d", &year, &month, &day, &hour, &minute, &second);
    }
    
    if(parsed == 6) {
        if(year >= 2000 && year <= 2099 &&
           month >= 1 && month <= 12 &&
           day >= 1 && day <= 31 &&
           hour >= 0 && hour <= 23 &&
           minute >= 0 && minute <= 59 &&
           second >= 0 && second <= 59) {

            // 用户输入的是东八区时间，需要转换为UTC时间存储到RTC
            // 东八区比UTC快8小时，所以UTC = 本地时间 - 8小时
            int utc_hour = hour - 8;
            int utc_day = day;
            int utc_month = month;
            int utc_year = year;

            // 处理跨日期的情况
            if(utc_hour < 0) {
                utc_hour += 24;
                utc_day--;

                // 处理跨月的情况
                if(utc_day < 1) {
                    utc_month--;
                    if(utc_month < 1) {
                        utc_month = 12;
                        utc_year--;
                    }
                    utc_day = get_days_in_month(utc_month, utc_year);
                }
            }

            extern rtc_parameter_struct rtc_initpara;

            // 转换为BCD格式（RTC硬件需要BCD格式）
            rtc_initpara.year = dec_to_bcd(utc_year - 2000);
            rtc_initpara.month = dec_to_bcd(utc_month);
            rtc_initpara.date = dec_to_bcd(utc_day);
            rtc_initpara.day_of_week = 1;
            rtc_initpara.hour = dec_to_bcd(utc_hour);
            rtc_initpara.minute = dec_to_bcd(minute);
            rtc_initpara.second = dec_to_bcd(second);
            rtc_initpara.factor_asyn = prescaler_a;
            rtc_initpara.factor_syn = prescaler_s;
            rtc_initpara.am_pm = RTC_AM;
              if(rtc_init(&rtc_initpara) == SUCCESS) {
                my_printf(DEBUG_USART, "RTC Config success\r\n");
                my_printf(DEBUG_USART, "Time: %04d-%02d-%02d %02d:%02d:%02d\r\n",
                         year, month, day, hour, minute, second);

                // 记录RTC配置成功日志
                char log_msg[64];
                sprintf(log_msg, "rtc config success to %04d-%02d-%02d %02d:%02d:%02d",
                       year, month, day, hour, minute, second);
                log_operation(log_msg);
            } else {
                my_printf(DEBUG_USART, "Failed to set RTC time\r\n");
            }
        } else {
            my_printf(DEBUG_USART, "Invalid time parameters\r\n");
        }
    } else {
        my_printf(DEBUG_USART, "Invalid time format\r\n");
        my_printf(DEBUG_USART, "Supported formats:\r\n");
        my_printf(DEBUG_USART, "  2025-01-01 15:00:10\r\n");
        my_printf(DEBUG_USART, "  2025/01/01 15:00:10\r\n");
        my_printf(DEBUG_USART, "  2025 01 01 15 00 10\r\n");
    }
}

/**
 * @brief 显示当前RTC时间（东八区）
 * @return 无返回值
 * @note 自动转换UTC时间为本地时间显示
 */
void rtc_show_current_time(void)
{
    uint32_t timestamp = get_unix_timestamp();
    local_time_t local_time = timestamp_to_local_time(timestamp);

    my_printf(DEBUG_USART, "Current Time: %04d-%02d-%02d %02d:%02d:%02d\r\n",
              local_time.year, local_time.month, local_time.day,
              local_time.hour, local_time.minute, local_time.second);
}

/**
 * @brief 显示RTC调试信息
 * @return 无返回值
 * @note 显示原始BCD寄存器值、UTC时间、本地时间和时间戳信息
 */
void rtc_debug_info(void)
{
    extern rtc_parameter_struct rtc_initpara;

    // 读取RTC原始寄存器值
    rtc_current_time_get(&rtc_initpara);

    my_printf(DEBUG_USART, "=== RTC Debug Info ===\r\n");
    my_printf(DEBUG_USART, "Raw RTC registers (BCD format):\r\n");
    my_printf(DEBUG_USART, "  Year: 0x%02X (%d)\r\n", rtc_initpara.year, bcd_to_dec(rtc_initpara.year));
    my_printf(DEBUG_USART, "  Month: 0x%02X (%d)\r\n", rtc_initpara.month, bcd_to_dec(rtc_initpara.month));
    my_printf(DEBUG_USART, "  Date: 0x%02X (%d)\r\n", rtc_initpara.date, bcd_to_dec(rtc_initpara.date));
    my_printf(DEBUG_USART, "  Hour: 0x%02X (%d)\r\n", rtc_initpara.hour, bcd_to_dec(rtc_initpara.hour));
    my_printf(DEBUG_USART, "  Minute: 0x%02X (%d)\r\n", rtc_initpara.minute, bcd_to_dec(rtc_initpara.minute));
    my_printf(DEBUG_USART, "  Second: 0x%02X (%d)\r\n", rtc_initpara.second, bcd_to_dec(rtc_initpara.second));

    uint16_t utc_year = 2000 + bcd_to_dec(rtc_initpara.year);
    uint8_t utc_month = bcd_to_dec(rtc_initpara.month);
    uint8_t utc_day = bcd_to_dec(rtc_initpara.date);
    uint8_t utc_hour = bcd_to_dec(rtc_initpara.hour);
    uint8_t utc_minute = bcd_to_dec(rtc_initpara.minute);
    uint8_t utc_second = bcd_to_dec(rtc_initpara.second);

    my_printf(DEBUG_USART, "UTC time (stored in RTC): %04d-%02d-%02d %02d:%02d:%02d\r\n",
              utc_year, utc_month, utc_day, utc_hour, utc_minute, utc_second);

    uint32_t timestamp = get_unix_timestamp();
    my_printf(DEBUG_USART, "Unix timestamp: %u\r\n", timestamp);

    local_time_t local_time = timestamp_to_local_time(timestamp);
    my_printf(DEBUG_USART, "Local time (UTC+8): %04d-%02d-%02d %02d:%02d:%02d\r\n",
              local_time.year, local_time.month, local_time.day,
              local_time.hour, local_time.minute, local_time.second);
    my_printf(DEBUG_USART, "Timezone offset: +8 hours\r\n");
    my_printf(DEBUG_USART, "=====================\r\n");
}

/* =================== 参数配置模块 =================== */

/**
 * @brief 手动格式化浮点数输出（备用函数）
 * @param usart_periph USART外设编号
 * @param prefix 前缀字符串
 * @param value 浮点数值
 * @param suffix 后缀字符串
 * @return 无返回值
 * @note 将浮点数分解为整数和小数部分输出，精度为2位小数
 */
void print_float_manual(uint32_t usart_periph, const char* prefix, float value, const char* suffix)
{
    int integer_part = (int)value;
    int decimal_part = (int)((value - integer_part) * 100);
    
    if(decimal_part < 0) decimal_part = -decimal_part;
    
    my_printf(usart_periph, "%s%d.%02d%s", prefix, integer_part, decimal_part, suffix);
}

/**
 * @brief 处理比例参数设置命令
 * @return 无返回值
 * @note 显示当前比例值并等待用户输入新值（0-100范围）
 */
void ratio_setting_handler(void)
{

    my_printf(DEBUG_USART, "ratio= %.1f\r\n", system_config.ratio_ch0);
    my_printf(DEBUG_USART, "Input value(0~100):\r\n");
    
    waiting_for_ratio_input = 1;
    log_operation("ratio config");
}

/**
 * @brief 处理限制参数设置命令
 * @return 无返回值
 * @note 显示当前限制值并等待用户输入新值（0-200范围）
 */
void limit_setting_handler(void)
{
    my_printf(DEBUG_USART, "limit= %.2f\r\n", system_config.limit_ch0);
    my_printf(DEBUG_USART, "Input value(0~200):\r\n");
    
    waiting_for_limit_input = 1;
    log_operation("limit config");
}

/**
 * @brief 处理配置保存命令
 * @return 无返回值
 * @note 将当前系统配置参数保存到Flash存储器
 */
void config_save_handler(void)
{
    // print_float_manual(DEBUG_USART, "ratio: ", system_config.ratio_ch0, "\r\n");
    // print_float_manual(DEBUG_USART, "limit = ", system_config.limit_ch0, "\r\n");
    
    my_printf(DEBUG_USART, "ratio: %.1f\r\n", system_config.ratio_ch0);
    my_printf(DEBUG_USART, "limit: %.2f\r\n", system_config.limit_ch0);

    if(write_config_to_flash(&system_config) == SUCCESS) {
        my_printf(DEBUG_USART, "save parameters to flash\r\n");
    } else {
        my_printf(DEBUG_USART, "Failed to save parameters to flash\r\n");
    }
}

/**
 * @brief 处理从Flash读取配置命令
 * @return 无返回值
 * @note 从Flash存储器读取配置参数并更新系统配置
 */
void config_read_flash_handler(void)
{
    config_params_t temp_config;
    
    if(read_config_from_flash(&temp_config) == SUCCESS) {
        system_config = temp_config;
        my_printf(DEBUG_USART, "read parameters from flash\r\n");
        // print_float_manual(DEBUG_USART, "ratio: ", system_config.ratio_ch0, "\r\n");
        // print_float_manual(DEBUG_USART, "limit = ", system_config.limit_ch0, "\r\n");
        my_printf(DEBUG_USART, "ratio: %.1f\r\n", system_config.ratio_ch0);
        my_printf(DEBUG_USART, "limit: %.2f\r\n", system_config.limit_ch0);
    } else {
        my_printf(DEBUG_USART, "Failed to read parameters from flash\r\n");
    }
}

/* =================== Flash操作模块 =================== */

/**
 * @brief 将配置参数写入Flash存储器
 * @param config 指向配置参数结构体的指针
 * @return SUCCESS/ERROR 写入成功或失败状态
 * @note 先擦除扇区再写入，写入后进行回读验证确保数据正确性
 */
ErrStatus write_config_to_flash(config_params_t* config)
{
    uint32_t config_flash_addr = 0x1000;
    
    spi_flash_sector_erase(config_flash_addr);
    
    spi_flash_wait_for_write_end();
    
    spi_flash_buffer_write((uint8_t*)config, config_flash_addr, sizeof(config_params_t));
    
    config_params_t read_back_config;
    spi_flash_buffer_read((uint8_t*)&read_back_config, config_flash_addr, sizeof(config_params_t));
    if((read_back_config.ratio_ch0 == config->ratio_ch0) && 
       (read_back_config.limit_ch0 == config->limit_ch0) &&
       (read_back_config.sample_period == config->sample_period)) {
        return SUCCESS;
    } else {
        return ERROR;
    }
}

/**
 * @brief 从Flash存储器读取配置参数
 * @param config 指向配置参数结构体的指针，用于存储读取的数据
 * @return SUCCESS/ERROR 读取成功或失败状态
 * @note 读取后验证参数范围有效性，无效数据返回ERROR
 */
ErrStatus read_config_from_flash(config_params_t* config)
{
    uint32_t config_flash_addr = 0x1000;
    
    spi_flash_buffer_read((uint8_t*)config, config_flash_addr, sizeof(config_params_t));
    if(config->ratio_ch0 >= 0.0f && config->ratio_ch0 <= 100.0f && 
       config->limit_ch0 >= 0.0f && config->limit_ch0 <= 200.0f) {
        return SUCCESS;
    } else {
        return ERROR;
    }
}

/* =================== 采样控制模块 =================== */

/**
 * @brief 处理采样启动命令
 * @return 无返回值
 * @note 启动周期性采样，设置采样周期并更新OLED显示
 */
void sampling_start_handler(void)
{
    sampling_state.is_sampling = 1;
    sampling_state.sample_period = system_config.sample_period;
    sampling_state.last_sample_time = 0;
    
    my_printf(DEBUG_USART, "Periodic Sampling\r\n");
    my_printf(DEBUG_USART, "sample cycle: %ds\r\n", sampling_state.sample_period);
    
    oled_task();

    log_operation("sample start - cycle 5s (command)");
}

/**
 * @brief 处理采样停止命令
 * @return 无返回值
 * @note 停止周期性采样，清除OLED显示并更新状态
 */
void sampling_stop_handler(void)
{
    sampling_state.is_sampling = 0;
    
    OLED_Clear();
    LED1_OFF;
    LED2_OFF;
    my_printf(DEBUG_USART, "Periodic Sampling Stop\r\n");
    
    oled_task();
    
    log_operation("sample stop (command)");
}

/**
 * @brief 处理数据隐藏命令
 * @return 无返回值
 * @note 启用加密数据格式输出，同时启用hideData存储模式
 */
void hide_data_handler(void)
{
    sampling_state.hide_mode = 1;
    storage_state.hide_storage_enabled = 1;  // 同时启用hide存储
    // my_printf(DEBUG_USART, "Data format hidden\r\n");
}

/**
 * @brief 处理数据取消隐藏命令
 * @return 无返回值
 * @note 恢复普通数据格式输出，同时禁用hideData存储模式
 */
void unhide_data_handler(void)
{
    sampling_state.hide_mode = 0;
    storage_state.hide_storage_enabled = 0;  // 同时禁用hide存储
    // my_printf(DEBUG_USART, "Data format restored\r\n");
}

/* =================== 数据输出模块 =================== */

/**
 * @brief 输出采样数据到串口
 * @param voltage 电压值
 * @param over_limit 是否超限标志（1=超限，0=正常）
 * @return 无返回值
 * @note 根据hide模式选择输出格式：普通格式（时间+电压）或加密格式（时间戳+电压十六进制）
 */
void print_sample_data(float voltage, uint8_t over_limit)
{
    store_sample_data(voltage, over_limit);
    
    if(over_limit) {
        store_overlimit_data(voltage);
    }
    
    if(sampling_state.hide_mode) {
        uint32_t timestamp = get_unix_timestamp();
        
        uint16_t voltage_int = (uint16_t)voltage;
        uint16_t voltage_frac = (uint16_t)((voltage - voltage_int) * 65536);
        
        uint32_t voltage_hex = (voltage_int << 16) | voltage_frac;
        
        if(over_limit) {
            my_printf(DEBUG_USART, "%08X%08X*\r\n", timestamp, voltage_hex);
        } else {
            my_printf(DEBUG_USART, "%08X%08X\r\n", timestamp, voltage_hex);
        }
    } else {
        uint32_t timestamp = get_unix_timestamp();
        local_time_t local_time = timestamp_to_local_time(timestamp);
        
        if(over_limit) {
            my_printf(DEBUG_USART, "%04d-%02d-%02d %02d:%02d:%02d ch0=", 
                     local_time.year, local_time.month, local_time.day,
                     local_time.hour, local_time.minute, local_time.second);
            my_printf(DEBUG_USART, "%.1fV OverLimit(%.1fV)!\r\n", voltage, system_config.limit_ch0);
            // print_float_manual(DEBUG_USART, "", voltage, "V OverLimit(");
            // print_float_manual(DEBUG_USART, "", system_config.limit_ch0, ")!\r\n");
        } else {
            my_printf(DEBUG_USART, "%04d-%02d-%02d %02d:%02d:%02d ch0=", 
                     local_time.year, local_time.month, local_time.day,
                     local_time.hour, local_time.minute, local_time.second);
            // print_float_manual(DEBUG_USART, "", voltage, "V\r\n");
            my_printf(DEBUG_USART, "%.1fV\r\n", voltage);
        }
    }
}

/* =================== 系统初始化模块 =================== */

// 设备ID存储地址（Flash）
#define DEVICE_ID_FLASH_ADDR    0x3000

// 默认设备ID（如果Flash中没有有效ID）
#define DEFAULT_DEVICE_ID       "2025-CIMC-2025136399"

/**
 * @brief 从Flash读取设备ID
 * @param device_id 存储设备ID的缓冲区
 * @param max_len 缓冲区最大长度
 * @return 无返回值
 * @note 如果Flash中无有效ID则使用默认ID并写入Flash
 */
void read_device_id_from_flash(char* device_id, uint16_t max_len)
{
    // 从Flash读取设备ID
    spi_flash_buffer_read((uint8_t*)device_id, DEVICE_ID_FLASH_ADDR, max_len);

    // 检查读取的ID是否有效（以"2025-CIMC-"开头）
    if(strncmp(device_id, "2025-CIMC-", 10) != 0) {
        // 如果无效，使用默认ID
        strncpy(device_id, DEFAULT_DEVICE_ID, max_len - 1);
        device_id[max_len - 1] = '\0';

        // 将默认ID写入Flash
        spi_flash_sector_erase(DEVICE_ID_FLASH_ADDR);
        spi_flash_wait_for_write_end();
        spi_flash_buffer_write((uint8_t*)device_id, DEVICE_ID_FLASH_ADDR, strlen(device_id) + 1);
    }
}

/**
 * @brief 写入设备ID到Flash
 * @param device_id 要写入的设备ID字符串
 * @return 无返回值
 * @note 先擦除扇区再写入设备ID数据
 */
void write_device_id_to_flash(const char* device_id)
{
    // 擦除扇区
    spi_flash_sector_erase(DEVICE_ID_FLASH_ADDR);
    spi_flash_wait_for_write_end();

    // 写入设备ID
    spi_flash_buffer_write((uint8_t*)device_id, DEVICE_ID_FLASH_ADDR, strlen(device_id) + 1);
}

/**
 * @brief 系统启动初始化
 * @return 无返回值
 * @note 执行系统上电初始化流程：读取设备ID、初始化存储系统、记录启动日志
 */
void system_startup_init(void)
{
    char device_id[32];
    
    oled_task();
  
    spi_flash_sector_erase(DEVICE_ID_FLASH_ADDR);
    
    // 1.1 系统上电打印
    my_printf(DEBUG_USART, "====system init====\r\n");

    // 1.2 从Flash中读取设备ID
    read_device_id_from_flash(device_id, sizeof(device_id));
    my_printf(DEBUG_USART, "Device_ID:%s\r\n", device_id);

    // 初始化存储系统
    storage_init();

    // 记录系统启动日志
    log_operation("system init");

    // 1.3 系统就绪打印
    my_printf(DEBUG_USART, "====system ready====\r\n");
}

/**
 * @brief 设备ID设置命令处理
 * @param new_id 新的设备ID字符串
 * @return 无返回值
 * @note 验证ID格式（2025-CIMC-XXX）后写入Flash，格式错误时提示正确格式
 */
void device_id_setting_handler(char* new_id)
{
    // 验证ID格式（应该以"2025-CIMC-"开头）
    if(strncmp(new_id, "2025-CIMC-", 10) == 0 && strlen(new_id) <= 30) {
        // 写入Flash
        write_device_id_to_flash(new_id);

        my_printf(DEBUG_USART, "Device ID updated: %s\r\n", new_id);
    } else {
        my_printf(DEBUG_USART, "Invalid Device ID format\r\n");
        my_printf(DEBUG_USART, "Format: 2025-CIMC-XXX (XXX is team number)\r\n");
        my_printf(DEBUG_USART, "Example: 2025-CIMC-001\r\n");
    }
}

/* =================== 电源管理模块 =================== */

/**
 * @brief 上电次数查询处理函数
 * @return 无返回值
 * @note 显示系统上电次数、设备ID和当前时间信息
 */
void power_count_handler(void)
{
    uint32_t power_count = storage_state.log_id;

    my_printf(DEBUG_USART, "=== Power Count Info ===\r\n");
    my_printf(DEBUG_USART, "Total power-on count: %u\r\n", power_count);
    my_printf(DEBUG_USART, "Current log ID: %u\r\n", power_count);

    // 显示设备ID
    char device_id[32];
    read_device_id_from_flash(device_id, sizeof(device_id));
    my_printf(DEBUG_USART, "Device ID: %s\r\n", device_id);

    // 显示当前时间
    uint32_t timestamp = get_unix_timestamp();
    local_time_t local_time = timestamp_to_local_time(timestamp);
    my_printf(DEBUG_USART, "Current time: %04d-%02d-%02d %02d:%02d:%02d (UTC+8)\r\n",
              local_time.year, local_time.month, local_time.day,
              local_time.hour, local_time.minute, local_time.second);

    my_printf(DEBUG_USART, "========================\r\n");
}

/**
 * @brief 上电次数重置处理函数
 * @return 无返回值
 * @note 重置系统上电次数为0，保存到Flash并记录重置操作日志
 */
void power_count_reset_handler(void)
{
    my_printf(DEBUG_USART, "=== Power Count Reset ===\r\n");

    // 显示当前上电次数
    uint32_t current_count = storage_state.log_id;
    my_printf(DEBUG_USART, "Current power-on count: %u\r\n", current_count);

    // 重置Log ID到0
    storage_state.log_id = 0;
    save_log_id_to_flash(1);  // 保存1到Flash，下次启动时会变成1

    my_printf(DEBUG_USART, "Power-on count has been reset to 0\r\n");
    my_printf(DEBUG_USART, "Next power-on will start from count 1\r\n");

    // 记录重置操作到日志
    log_operation("power count reset");

    my_printf(DEBUG_USART, "=========================\r\n");
    my_printf(DEBUG_USART, "Note: Reset will take effect after next power-on\r\n");
}

