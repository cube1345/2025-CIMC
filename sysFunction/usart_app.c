
#include "ir_cmic_gd32f470vet6.h"

/* =================== ȫ�ֱ������� =================== */

__IO uint16_t tx_count = 0;
__IO uint8_t rx_flag = 0;
uint8_t uart_dma_buffer[512] = {0};
extern __IO uint32_t prescaler_a, prescaler_s;
extern uint16_t adc_value[1];

// ȫ�����ò���
config_params_t system_config = {1.0f, 1.0f, 5}; // Ĭ��ֵ��ratio=1.0, limit=1.0, period=5s

// ����״̬
sampling_state_t sampling_state = {0, 5, 0, 0}; 

storage_state_t storage_state = {0, 0, 0, 0, 0};

static volatile uint8_t waiting_for_ratio_input = 0;
static volatile uint8_t waiting_for_limit_input = 0;
static volatile uint8_t waiting_for_rtc_input = 0;

/* =================== USARTͨ��ģ�� =================== */

/**
 * @brief ��ʽ�������ָ��USART�˿�
 *
 * ����printf�Ĺ��ܣ�֧�ָ�ʽ���ַ��������ָ����USART�˿ڡ�
 * �ڲ�ʹ��512�ֽڻ�������֧�ֿɱ�����б�������ѯ��ʽ�������ݣ�
 * ȷ��ÿ���ַ����ɹ����ͺ��ٷ�����һ���ַ���
 *
 * @param usart_periph USART�����ţ���USART0��USART1�ȣ�����������Ч��USART����
 * @param format ��ʽ���ַ�����֧�ֱ�׼printf��ʽ˵������%d��%s��%f�ȣ�
 * @param ... �ɱ�����б���Ӧ��ʽ���ַ����е�ռλ��
 * @return ʵ�ʷ��͵��ַ������������ʽ��ʧ���򷵻ظ�ֵ
 * @note ���֧��512�ַ���������������ֽ����ض�
 * @warning ȷ��USART�˿�����ȷ��ʼ�����ٵ��ô˺��������͹����л������ȴ�
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
 * @brief UART��������
 *
 * ���UART���ձ�־������ͨ��DMA���յ����������ݡ�
 * �Զ�ȥ�������ַ���ĩβ�Ļ��з����س����Ϳո�
 * Ȼ���������������н�����ִ�С�������ɺ���ս��ջ�������
 *
 * @return �޷���ֵ
 * @note �˺���Ӧ����ѭ���������Ե��ã�����ȫ�ֱ���rx_flag��uart_dma_buffer
 * @warning ȷ��UART DMA��������ȷ���ã���uart_dma_buffer��С�㹻
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

/* =================== �����ģ�� =================== */

/**
 * @brief ������յ��Ĵ�������
 * @param command ���յ��������ַ�������ȥ�����з��Ϳո�
 * @return �޷���ֵ
 * @note ֧�ֽ���ʽ��������Ͷ���ϵͳ����������ԡ����á��������Ƶ�
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

    // ����RTCʱ������
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
        // 2.1 ���ڷ���"Input Datetime"
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
    // �豸ID��������
    else if(strncmp(command, "device id", 9) == 0) {
        char* id_param = command + 9;
        while(*id_param == ' ') id_param++; // �����ո�
        if(*id_param != '\0') {
            device_id_setting_handler(id_param);
        } else {
            // ��ʾ��ǰ�豸ID
            char current_id[32];
            read_device_id_from_flash(current_id, sizeof(current_id));
            my_printf(DEBUG_USART, "Current Device ID: %s\r\n", current_id);
            my_printf(DEBUG_USART, "Usage: device id 2025-CIMC-XXX\r\n");
        }
    }
    // RTC��������
    else if(strcmp(command, "rtc debug") == 0) {
        rtc_debug_info();
    }
    // �ϵ������ѯ����
    else if(strcmp(command, "power count") == 0) {
        power_count_handler();
    }
    // �ϵ������������
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

/* =================== ϵͳ����ģ�� =================== */

/**
 * @brief ִ��ϵͳӲ���Լ����
 * @return �޷���ֵ
 * @note ���Flash��TF����RTC״̬��������Խ������¼������־
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

/* =================== RTC����ģ�� =================== */

/**
 * @brief ����RTCʱ����������
 * @param time_str ʱ���ַ�����֧�ֶ��ָ�ʽ��YYYY-MM-DD HH:MM:SS�ȣ�
 * @return �޷���ֵ
 * @note �Զ�ת��������ʱ��ΪUTCʱ��洢��֧�ֿ����ڴ���
 */
void rtc_config_handler(char* time_str)
{
    int year, month, day, hour, minute, second;

    int parsed = 0;

    // 2.2 ֧�ֱ�׼ʱ���ʽ��"2025-01-01 15:00:10"
    parsed = sscanf(time_str, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);

    // ���ø�ʽ1���ո�ָ�
    if(parsed != 6) {
        parsed = sscanf(time_str, "%d %d %d %d %d %d", &year, &month, &day, &hour, &minute, &second);
    }

    // ���ø�ʽ2��б�ָܷ�
    if(parsed != 6) {
        parsed = sscanf(time_str, "%d/%d/%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
    }

    // ���ø�ʽ3����ָܷ�ʱ��
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

            // �û�������Ƕ�����ʱ�䣬��Ҫת��ΪUTCʱ��洢��RTC
            // ��������UTC��8Сʱ������UTC = ����ʱ�� - 8Сʱ
            int utc_hour = hour - 8;
            int utc_day = day;
            int utc_month = month;
            int utc_year = year;

            // ��������ڵ����
            if(utc_hour < 0) {
                utc_hour += 24;
                utc_day--;

                // ������µ����
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

            // ת��ΪBCD��ʽ��RTCӲ����ҪBCD��ʽ��
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

                // ��¼RTC���óɹ���־
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
 * @brief ��ʾ��ǰRTCʱ�䣨��������
 * @return �޷���ֵ
 * @note �Զ�ת��UTCʱ��Ϊ����ʱ����ʾ
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
 * @brief ��ʾRTC������Ϣ
 * @return �޷���ֵ
 * @note ��ʾԭʼBCD�Ĵ���ֵ��UTCʱ�䡢����ʱ���ʱ�����Ϣ
 */
void rtc_debug_info(void)
{
    extern rtc_parameter_struct rtc_initpara;

    // ��ȡRTCԭʼ�Ĵ���ֵ
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

/* =================== ��������ģ�� =================== */

/**
 * @brief �ֶ���ʽ����������������ú�����
 * @param usart_periph USART������
 * @param prefix ǰ׺�ַ���
 * @param value ������ֵ
 * @param suffix ��׺�ַ���
 * @return �޷���ֵ
 * @note ���������ֽ�Ϊ������С���������������Ϊ2λС��
 */
void print_float_manual(uint32_t usart_periph, const char* prefix, float value, const char* suffix)
{
    int integer_part = (int)value;
    int decimal_part = (int)((value - integer_part) * 100);
    
    if(decimal_part < 0) decimal_part = -decimal_part;
    
    my_printf(usart_periph, "%s%d.%02d%s", prefix, integer_part, decimal_part, suffix);
}

/**
 * @brief �������������������
 * @return �޷���ֵ
 * @note ��ʾ��ǰ����ֵ���ȴ��û�������ֵ��0-100��Χ��
 */
void ratio_setting_handler(void)
{

    my_printf(DEBUG_USART, "ratio= %.1f\r\n", system_config.ratio_ch0);
    my_printf(DEBUG_USART, "Input value(0~100):\r\n");
    
    waiting_for_ratio_input = 1;
    log_operation("ratio config");
}

/**
 * @brief �������Ʋ�����������
 * @return �޷���ֵ
 * @note ��ʾ��ǰ����ֵ���ȴ��û�������ֵ��0-200��Χ��
 */
void limit_setting_handler(void)
{
    my_printf(DEBUG_USART, "limit= %.2f\r\n", system_config.limit_ch0);
    my_printf(DEBUG_USART, "Input value(0~200):\r\n");
    
    waiting_for_limit_input = 1;
    log_operation("limit config");
}

/**
 * @brief �������ñ�������
 * @return �޷���ֵ
 * @note ����ǰϵͳ���ò������浽Flash�洢��
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
 * @brief �����Flash��ȡ��������
 * @return �޷���ֵ
 * @note ��Flash�洢����ȡ���ò���������ϵͳ����
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

/* =================== Flash����ģ�� =================== */

/**
 * @brief �����ò���д��Flash�洢��
 * @param config ָ�����ò����ṹ���ָ��
 * @return SUCCESS/ERROR д��ɹ���ʧ��״̬
 * @note �Ȳ���������д�룬д�����лض���֤ȷ��������ȷ��
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
 * @brief ��Flash�洢����ȡ���ò���
 * @param config ָ�����ò����ṹ���ָ�룬���ڴ洢��ȡ������
 * @return SUCCESS/ERROR ��ȡ�ɹ���ʧ��״̬
 * @note ��ȡ����֤������Χ��Ч�ԣ���Ч���ݷ���ERROR
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

/* =================== ��������ģ�� =================== */

/**
 * @brief ���������������
 * @return �޷���ֵ
 * @note ���������Բ��������ò������ڲ�����OLED��ʾ
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
 * @brief �������ֹͣ����
 * @return �޷���ֵ
 * @note ֹͣ�����Բ��������OLED��ʾ������״̬
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
 * @brief ����������������
 * @return �޷���ֵ
 * @note ���ü������ݸ�ʽ�����ͬʱ����hideData�洢ģʽ
 */
void hide_data_handler(void)
{
    sampling_state.hide_mode = 1;
    storage_state.hide_storage_enabled = 1;  // ͬʱ����hide�洢
    // my_printf(DEBUG_USART, "Data format hidden\r\n");
}

/**
 * @brief ��������ȡ����������
 * @return �޷���ֵ
 * @note �ָ���ͨ���ݸ�ʽ�����ͬʱ����hideData�洢ģʽ
 */
void unhide_data_handler(void)
{
    sampling_state.hide_mode = 0;
    storage_state.hide_storage_enabled = 0;  // ͬʱ����hide�洢
    // my_printf(DEBUG_USART, "Data format restored\r\n");
}

/* =================== �������ģ�� =================== */

/**
 * @brief ����������ݵ�����
 * @param voltage ��ѹֵ
 * @param over_limit �Ƿ��ޱ�־��1=���ޣ�0=������
 * @return �޷���ֵ
 * @note ����hideģʽѡ�������ʽ����ͨ��ʽ��ʱ��+��ѹ������ܸ�ʽ��ʱ���+��ѹʮ�����ƣ�
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

/* =================== ϵͳ��ʼ��ģ�� =================== */

// �豸ID�洢��ַ��Flash��
#define DEVICE_ID_FLASH_ADDR    0x3000

// Ĭ���豸ID�����Flash��û����ЧID��
#define DEFAULT_DEVICE_ID       "2025-CIMC-2025136399"

/**
 * @brief ��Flash��ȡ�豸ID
 * @param device_id �洢�豸ID�Ļ�����
 * @param max_len ��������󳤶�
 * @return �޷���ֵ
 * @note ���Flash������ЧID��ʹ��Ĭ��ID��д��Flash
 */
void read_device_id_from_flash(char* device_id, uint16_t max_len)
{
    // ��Flash��ȡ�豸ID
    spi_flash_buffer_read((uint8_t*)device_id, DEVICE_ID_FLASH_ADDR, max_len);

    // ����ȡ��ID�Ƿ���Ч����"2025-CIMC-"��ͷ��
    if(strncmp(device_id, "2025-CIMC-", 10) != 0) {
        // �����Ч��ʹ��Ĭ��ID
        strncpy(device_id, DEFAULT_DEVICE_ID, max_len - 1);
        device_id[max_len - 1] = '\0';

        // ��Ĭ��IDд��Flash
        spi_flash_sector_erase(DEVICE_ID_FLASH_ADDR);
        spi_flash_wait_for_write_end();
        spi_flash_buffer_write((uint8_t*)device_id, DEVICE_ID_FLASH_ADDR, strlen(device_id) + 1);
    }
}

/**
 * @brief д���豸ID��Flash
 * @param device_id Ҫд����豸ID�ַ���
 * @return �޷���ֵ
 * @note �Ȳ���������д���豸ID����
 */
void write_device_id_to_flash(const char* device_id)
{
    // ��������
    spi_flash_sector_erase(DEVICE_ID_FLASH_ADDR);
    spi_flash_wait_for_write_end();

    // д���豸ID
    spi_flash_buffer_write((uint8_t*)device_id, DEVICE_ID_FLASH_ADDR, strlen(device_id) + 1);
}

/**
 * @brief ϵͳ������ʼ��
 * @return �޷���ֵ
 * @note ִ��ϵͳ�ϵ��ʼ�����̣���ȡ�豸ID����ʼ���洢ϵͳ����¼������־
 */
void system_startup_init(void)
{
    char device_id[32];
    
    oled_task();
  
    spi_flash_sector_erase(DEVICE_ID_FLASH_ADDR);
    
    // 1.1 ϵͳ�ϵ��ӡ
    my_printf(DEBUG_USART, "====system init====\r\n");

    // 1.2 ��Flash�ж�ȡ�豸ID
    read_device_id_from_flash(device_id, sizeof(device_id));
    my_printf(DEBUG_USART, "Device_ID:%s\r\n", device_id);

    // ��ʼ���洢ϵͳ
    storage_init();

    // ��¼ϵͳ������־
    log_operation("system init");

    // 1.3 ϵͳ������ӡ
    my_printf(DEBUG_USART, "====system ready====\r\n");
}

/**
 * @brief �豸ID���������
 * @param new_id �µ��豸ID�ַ���
 * @return �޷���ֵ
 * @note ��֤ID��ʽ��2025-CIMC-XXX����д��Flash����ʽ����ʱ��ʾ��ȷ��ʽ
 */
void device_id_setting_handler(char* new_id)
{
    // ��֤ID��ʽ��Ӧ����"2025-CIMC-"��ͷ��
    if(strncmp(new_id, "2025-CIMC-", 10) == 0 && strlen(new_id) <= 30) {
        // д��Flash
        write_device_id_to_flash(new_id);

        my_printf(DEBUG_USART, "Device ID updated: %s\r\n", new_id);
    } else {
        my_printf(DEBUG_USART, "Invalid Device ID format\r\n");
        my_printf(DEBUG_USART, "Format: 2025-CIMC-XXX (XXX is team number)\r\n");
        my_printf(DEBUG_USART, "Example: 2025-CIMC-001\r\n");
    }
}

/* =================== ��Դ����ģ�� =================== */

/**
 * @brief �ϵ������ѯ������
 * @return �޷���ֵ
 * @note ��ʾϵͳ�ϵ�������豸ID�͵�ǰʱ����Ϣ
 */
void power_count_handler(void)
{
    uint32_t power_count = storage_state.log_id;

    my_printf(DEBUG_USART, "=== Power Count Info ===\r\n");
    my_printf(DEBUG_USART, "Total power-on count: %u\r\n", power_count);
    my_printf(DEBUG_USART, "Current log ID: %u\r\n", power_count);

    // ��ʾ�豸ID
    char device_id[32];
    read_device_id_from_flash(device_id, sizeof(device_id));
    my_printf(DEBUG_USART, "Device ID: %s\r\n", device_id);

    // ��ʾ��ǰʱ��
    uint32_t timestamp = get_unix_timestamp();
    local_time_t local_time = timestamp_to_local_time(timestamp);
    my_printf(DEBUG_USART, "Current time: %04d-%02d-%02d %02d:%02d:%02d (UTC+8)\r\n",
              local_time.year, local_time.month, local_time.day,
              local_time.hour, local_time.minute, local_time.second);

    my_printf(DEBUG_USART, "========================\r\n");
}

/**
 * @brief �ϵ�������ô�����
 * @return �޷���ֵ
 * @note ����ϵͳ�ϵ����Ϊ0�����浽Flash����¼���ò�����־
 */
void power_count_reset_handler(void)
{
    my_printf(DEBUG_USART, "=== Power Count Reset ===\r\n");

    // ��ʾ��ǰ�ϵ����
    uint32_t current_count = storage_state.log_id;
    my_printf(DEBUG_USART, "Current power-on count: %u\r\n", current_count);

    // ����Log ID��0
    storage_state.log_id = 0;
    save_log_id_to_flash(1);  // ����1��Flash���´�����ʱ����1

    my_printf(DEBUG_USART, "Power-on count has been reset to 0\r\n");
    my_printf(DEBUG_USART, "Next power-on will start from count 1\r\n");

    // ��¼���ò�������־
    log_operation("power count reset");

    my_printf(DEBUG_USART, "=========================\r\n");
    my_printf(DEBUG_USART, "Note: Reset will take effect after next power-on\r\n");
}

