#include "ir_cmic_gd32f470vet6.h"
#include "usart_app.h"
#include "ff.h"
#include "sd_app.h"

// 原有的SD卡测试变量
FIL fdst;
uint16_t i = 0, count, result = 0;
UINT br, bw;
sd_card_info_struct sd_cardinfo;
BYTE buffer[128];
BYTE filebuffer[128];

// 文件系统和存储相关全局变量
FATFS fs;
extern config_params_t system_config;
extern storage_state_t storage_state;

// 文件句柄变量
FIL current_log_file;
FIL current_sample_file;
FIL current_overlimit_file;
FIL current_hidedata_file;

// 文件打开标志
uint8_t log_file_open = 0;
uint8_t sample_file_open = 0;
uint8_t overlimit_file_open = 0;
uint8_t hidedata_file_open = 0;

ErrStatus memory_compare(uint8_t* src, uint8_t* dst, uint16_t length) 
{
    while(length --){
        if(*src++ != *dst++)
            return ERROR;
    }
    return SUCCESS;
}

void sd_fatfs_init(void)
{
    nvic_irq_enable(SDIO_IRQn, 0, 0);					// 使能SDIO中断，优先级为0
}

/**
 * @brief       通过串口打印SD卡相关信息
 * @param       无
 * @retval      无
 */
void card_info_get(void)
{
    sd_card_info_struct sd_cardinfo;      // SD卡信息结构体
    sd_error_enum status;                 // SD卡操作状态
    uint32_t block_count, block_size;
    
    // 获取SD卡信息
    status = sd_card_information_get(&sd_cardinfo);
    
    if(SD_OK == status)
    {
        my_printf(DEBUG_USART, "\r\n*** SD Card Info ***\r\n");
        
        // 打印卡类型
        switch(sd_cardinfo.card_type)
        {
            case SDIO_STD_CAPACITY_SD_CARD_V1_1:
                my_printf(DEBUG_USART, "Card Type: Standard Capacity SD Card V1.1\r\n");
                break;
            case SDIO_STD_CAPACITY_SD_CARD_V2_0:
                my_printf(DEBUG_USART, "Card Type: Standard Capacity SD Card V2.0\r\n");
                break;
            case SDIO_HIGH_CAPACITY_SD_CARD:
                my_printf(DEBUG_USART, "Card Type: High Capacity SD Card\r\n");
                break;
            case SDIO_MULTIMEDIA_CARD:
                my_printf(DEBUG_USART, "Card Type: Multimedia Card\r\n");
                break;
            case SDIO_HIGH_CAPACITY_MULTIMEDIA_CARD:
                my_printf(DEBUG_USART, "Card Type: High Capacity Multimedia Card\r\n");
                break;
            case SDIO_HIGH_SPEED_MULTIMEDIA_CARD:
                my_printf(DEBUG_USART, "Card Type: High Speed Multimedia Card\r\n");
                break;
            default:
                my_printf(DEBUG_USART, "Card Type: Unknown\r\n");
                break;
        }
        
        // 打印卡容量和块大小
        block_count = (sd_cardinfo.card_csd.c_size + 1) * 1024;
        block_size = 512;
        my_printf(DEBUG_USART,"\r\n## Device size is %dKB (%.2fGB)##", sd_card_capacity_get(), sd_card_capacity_get() / 1024.0f / 1024.0f);
        my_printf(DEBUG_USART,"\r\n## Block size is %dB ##", block_size);
        my_printf(DEBUG_USART,"\r\n## Block count is %d ##", block_count);
        
        // 打印制造商ID和产品名称
        my_printf(DEBUG_USART, "Manufacturer ID: 0x%X\r\n", sd_cardinfo.card_cid.mid);
        my_printf(DEBUG_USART, "OEM/Application ID: 0x%X\r\n", sd_cardinfo.card_cid.oid);
        
        // 打印产品名称 (PNM)
        uint8_t pnm[6];
        pnm[0] = (sd_cardinfo.card_cid.pnm0 >> 24) & 0xFF;
        pnm[1] = (sd_cardinfo.card_cid.pnm0 >> 16) & 0xFF;
        pnm[2] = (sd_cardinfo.card_cid.pnm0 >> 8) & 0xFF;
        pnm[3] = sd_cardinfo.card_cid.pnm0 & 0xFF;
        pnm[4] = sd_cardinfo.card_cid.pnm1 & 0xFF;
        pnm[5] = '\0';
        my_printf(DEBUG_USART, "Product Name: %s\r\n", pnm);
        
        // 打印产品版本和序列号
        my_printf(DEBUG_USART, "Product Revision: %d.%d\r\n", (sd_cardinfo.card_cid.prv >> 4) & 0x0F, sd_cardinfo.card_cid.prv & 0x0F);
        // 序列号以无符号方式显示，避免负数
        my_printf(DEBUG_USART, "Product Serial Number: 0x%08X\r\n", sd_cardinfo.card_cid.psn);
        
        // 打印CSD版本和其它CSD信息
        my_printf(DEBUG_USART, "CSD Version: %d.0\r\n", sd_cardinfo.card_csd.csd_struct + 1);
        
    }
    else
    {
        my_printf(DEBUG_USART, "\r\nFailed to get SD card information, error code: %d\r\n", status);
    }
}

void config_read_handler(void)
{
    FIL config_file;
    FRESULT result;
    char line_buffer[128];
    
    // 尝试打开config.ini文件（使用全局挂载的文件系统）
    result = f_open(&config_file, "/config.ini", FA_READ);
    if(result != FR_OK) {
        my_printf(DEBUG_USART, "config.ini file not found.\r\n");
        return;
    }
  
    // 解析配置文件
    uint8_t ratio_section_found = 0;
    uint8_t limit_section_found = 0;
    uint8_t ratio_parsed = 0;
    uint8_t limit_parsed = 0;
    
    while(f_gets(line_buffer, sizeof(line_buffer), &config_file) != NULL) {
        // 移除行末的换行符
        char* newline = strchr(line_buffer, '\n');
        if(newline) *newline = '\0';
        char* carriage = strchr(line_buffer, '\r');
        if(carriage) *carriage = '\0';
        
        // 跳过空行和注释
        if(strlen(line_buffer) == 0 || line_buffer[0] == ';' || line_buffer[0] == '#') {
            continue;
        }
        
        // 检查是否是节标题
        if(line_buffer[0] == '[') {
            if(strstr(line_buffer, "[Ratio]") != NULL) {
                ratio_section_found = 1;
                limit_section_found = 0;
            } else if(strstr(line_buffer, "[Limit]") != NULL) {
                limit_section_found = 1;
                ratio_section_found = 0;
            } else {
                ratio_section_found = 0;
                limit_section_found = 0;
            }
        }
        // 解析配置项
        else if(ratio_section_found && strstr(line_buffer, "Ch0") != NULL) {
            char* equal_sign = strchr(line_buffer, '=');
            if(equal_sign != NULL) {
                equal_sign++; // 跳过等号
                while(*equal_sign == ' ') equal_sign++; // 跳过空格
                system_config.ratio_ch0 = atof(equal_sign);
                ratio_parsed = 1;
            }
        }
        else if(limit_section_found && strstr(line_buffer, "Ch0") != NULL) {
            char* equal_sign = strchr(line_buffer, '=');
            if(equal_sign != NULL) {
                equal_sign++; // 跳过等号
                while(*equal_sign == ' ') equal_sign++; // 跳过空格
                system_config.limit_ch0 = atof(equal_sign);
                limit_parsed = 1;
            }
        }
    }
    
    // 关闭文件
    f_close(&config_file);
    
    // 检查是否成功解析了所有必要的参数
    if(ratio_parsed && limit_parsed) {
        // 输出读取的配置 - 手动处理浮点数显示，保留两位小数
        // int ratio_int = (int)system_config.ratio_ch0;
        // int ratio_frac = (int)((system_config.ratio_ch0 - ratio_int) * 100);
        // my_printf(DEBUG_USART, "Ratio = %d.%02d\r\n", ratio_int, ratio_frac);
        
        // int limit_int = (int)system_config.limit_ch0;
        // int limit_frac = (int)((system_config.limit_ch0 - limit_int) * 100);
        // my_printf(DEBUG_USART, "Limit = %d.%02d\r\n", limit_int, limit_frac);
        my_printf(DEBUG_USART, "ratio: %.1f\r\n", system_config.ratio_ch0);
        my_printf(DEBUG_USART, "limit: %.2f\r\n", system_config.limit_ch0);
        
        // 将配置写入Flash (这里简化处理，实际应用中需要选择合适的Flash地址)
        if(write_config_to_flash(&system_config) == SUCCESS) {
            my_printf(DEBUG_USART, "config read success\r\n");
        } else {
//            my_printf(DEBUG_USART, "Failed to write config to flash\r\n");
        }
    } else {
//        my_printf(DEBUG_USART, "Invalid config file format\r\n");
    }
}

// =================== 数据存储相关函数 ===================

// 获取当前时间的日期时间字符串（格式：YYYYMMDDHHMMSS）
void get_datetime_string(char* datetime_str)
{
    uint32_t timestamp = get_unix_timestamp();
    local_time_t local_time = timestamp_to_local_time(timestamp);
    
    sprintf(datetime_str, "%04d%02d%02d%02d%02d%02d",
            local_time.year, local_time.month, local_time.day,
            local_time.hour, local_time.minute, local_time.second);
}

// 创建存储目录
void create_storage_directories(void)
{
    FRESULT result;

    // 静默创建存储文件夹
    f_mkdir("sample");
    f_mkdir("overLimit");
    f_mkdir("log");
    f_mkdir("hideData");

    // 静默模式，不输出任何信息
}

// 从Flash读取日志ID
uint32_t get_next_log_id(void)
{
    uint32_t log_id_flash_addr = 0x2000; // 使用不同的Flash地址存储日志ID
    uint32_t stored_log_id;
    
    spi_flash_buffer_read((uint8_t*)&stored_log_id, log_id_flash_addr, sizeof(uint32_t));
    
    // 检查读取的值是否有效
    if(stored_log_id == 0xFFFFFFFF || stored_log_id > 10000) {
        stored_log_id = 0; // 首次使用或数据无效
    }
    
    return stored_log_id;
}

// 保存日志ID到Flash
void save_log_id_to_flash(uint32_t log_id)
{
    uint32_t log_id_flash_addr = 0x2000;
    
    // 擦除扇区
    spi_flash_sector_erase(log_id_flash_addr);
    spi_flash_wait_for_write_end();
    
    // 写入日志ID
    spi_flash_buffer_write((uint8_t*)&log_id, log_id_flash_addr, sizeof(uint32_t));
}

// 存储系统初始化
void storage_init(void)
{
    FRESULT result;
    
    // 确保SD卡已经初始化
    DSTATUS sd_status = disk_initialize(0);
    if(sd_status != RES_OK) {
        return;
    }
    
    // 挂载文件系统并保持挂载状态
    result = f_mount(0, &fs);
    if(result != FR_OK) {
//        my_printf(DEBUG_USART, "Failed to mount TF card for storage init: %d\r\n", result);
        return;
    }

    // 创建存储目录 (静默模式)
    create_storage_directories();

    // 获取日志ID
    storage_state.log_id = get_next_log_id();

    // 创建日志文件 - 使用log文件夹
    char log_filename[64];
    sprintf(log_filename, "log/log%u.txt", storage_state.log_id);

    result = f_open(&current_log_file, log_filename, FA_CREATE_ALWAYS | FA_WRITE);
    if(result == FR_OK) {
        log_file_open = 1;
    } else {
    }
    
    // 保存递增的日志ID到Flash
    save_log_id_to_flash(storage_state.log_id + 1);
}

// 记录操作日志
void log_operation(const char* operation)
{
    
    if(!log_file_open) return;
    
    uint32_t timestamp = get_unix_timestamp();
    local_time_t local_time = timestamp_to_local_time(timestamp);
    
    char log_entry[256];
    // 格式：2025-01-01 10:00:01 system init
    sprintf(log_entry, "%04d-%02d-%02d %02d:%02d:%02d %s\r\n",
            local_time.year, local_time.month, local_time.day,
            local_time.hour, local_time.minute, local_time.second,
            operation);
    
    UINT bytes_written;
    FRESULT result = f_write(&current_log_file, log_entry, strlen(log_entry), &bytes_written);
    if(result == FR_OK && bytes_written == strlen(log_entry)) {
        f_sync(&current_log_file);
    } else {
//        my_printf(DEBUG_USART, "Log write error: %d, bytes: %d/%d\r\n", 
//                 result, bytes_written, strlen(log_entry));
    }
}

// 存储采样数据
void store_sample_data(float voltage, uint8_t over_limit)
{
    // 如果启用了加密存储，普通采样数据不存储到sample文件夹
    if(storage_state.hide_storage_enabled) {
        store_hidedata(voltage, over_limit);
        return;
    }
    
    FRESULT result;
    
	
	
	
    // 如果需要新建文件或文件未打开
    if(!sample_file_open || storage_state.sample_count >= 10) {
        // 关闭当前文件
        if(sample_file_open) {
            f_close(&current_sample_file);
            sample_file_open = 0;
        }
        
        // 创建新文件 - 使用sample文件夹和正确的文件名格式
        char datetime_str[16];
        get_datetime_string(datetime_str);
        char filename[64];
        sprintf(filename, "sample/sampleData%s.txt", datetime_str);
        
        result = f_open(&current_sample_file, filename, FA_CREATE_ALWAYS | FA_WRITE);
        if(result == FR_OK) {
            sample_file_open = 1;
            storage_state.sample_count = 0;
            
            // sample文件不需要文件头，直接写入数据
            
            // my_printf(DEBUG_USART, "Sample file created: %s\r\n", filename);
        } else {
            // my_printf(DEBUG_USART, "Failed to create sample file, error: %d\r\n", result);
            return;
        }
    }
    
    // 写入数据
    if(sample_file_open) {
        uint32_t timestamp = get_unix_timestamp();
        local_time_t local_time = timestamp_to_local_time(timestamp);
        
        char data_line[128];
        // 格式：2025-01-01 00:30:10 1.5V
        sprintf(data_line, "%04d-%02d-%02d %02d:%02d:%02d %.1fV\r\n",
                local_time.year, local_time.month, local_time.day,
                local_time.hour, local_time.minute, local_time.second,
                voltage);
        
        UINT bytes_written;
        result = f_write(&current_sample_file, data_line, strlen(data_line), &bytes_written);
        if(result == FR_OK && bytes_written == strlen(data_line)) {
            f_sync(&current_sample_file);
            storage_state.sample_count++;
        } else {
            // my_printf(DEBUG_USART, "Sample data write error: %d, bytes: %d/%d\r\n", 
            //         result, bytes_written, strlen(data_line));
        }
    }
}

// 存储超阈值数据
void store_overlimit_data(float voltage)
{
    FRESULT result;
    
    // 如果需要新建文件或文件未打开
    if(!overlimit_file_open || storage_state.overlimit_count >= 10) {
        // 关闭当前文件
        if(overlimit_file_open) {
            f_close(&current_overlimit_file);
            overlimit_file_open = 0;
        }
        
        // 创建新文件 - 使用overLimit文件夹和正确的文件名格式
        char datetime_str[16];
        get_datetime_string(datetime_str);
        char filename[64];
        sprintf(filename, "overLimit/overLimit%s.txt", datetime_str);
        
        result = f_open(&current_overlimit_file, filename, FA_CREATE_ALWAYS | FA_WRITE);
        if(result == FR_OK) {
            overlimit_file_open = 1;
            storage_state.overlimit_count = 0;
            
            // overLimit文件不需要文件头，直接写入数据
            
            // my_printf(DEBUG_USART, "OverLimit file created: %s\r\n", filename);
        } else {
            // my_printf(DEBUG_USART, "Failed to create overlimit file, error: %d\r\n", result);
            return;
        }
    }
    
    // 写入数据
    if(overlimit_file_open) {
        uint32_t timestamp = get_unix_timestamp();
        local_time_t local_time = timestamp_to_local_time(timestamp);
        
        char data_line[128];
        // 格式：2025-01-01 00:30:10 30V limit 10V
        sprintf(data_line, "%04d-%02d-%02d %02d:%02d:%02d %.0fV limit %.0fV\r\n",
                local_time.year, local_time.month, local_time.day,
                local_time.hour, local_time.minute, local_time.second,
                voltage, system_config.limit_ch0);
        
        UINT bytes_written;
        result = f_write(&current_overlimit_file, data_line, strlen(data_line), &bytes_written);
        if(result == FR_OK && bytes_written == strlen(data_line)) {
            f_sync(&current_overlimit_file);
            storage_state.overlimit_count++;
        } else {
            // my_printf(DEBUG_USART, "OverLimit data write error: %d, bytes: %d/%d\r\n", 
            //          result, bytes_written, strlen(data_line));
        }
    }
}

// 存储加密数据
void store_hidedata(float voltage, uint8_t over_limit)
{
    FRESULT result;
    
    // 如果需要新建文件或文件未打开
    if(!hidedata_file_open || storage_state.hidedata_count >= 10) {
        // 关闭当前文件
        if(hidedata_file_open) {
            f_close(&current_hidedata_file);
            hidedata_file_open = 0;
        }
        
        // 创建新文件 - 使用hideData文件夹和正确的文件名格式
        char datetime_str[16];
        get_datetime_string(datetime_str);
        char filename[64];
        sprintf(filename, "hideData/hideData%s.txt", datetime_str);
        
        result = f_open(&current_hidedata_file, filename, FA_CREATE_ALWAYS | FA_WRITE);
        if(result == FR_OK) {
            hidedata_file_open = 1;
            storage_state.hidedata_count = 0;
            
            // hideData文件不需要文件头，直接写入数据
            
            // 静默模式，不显示hideData文件创建信息
        } else {
            // my_printf(DEBUG_USART, "Failed to create hidedata file, error: %d\r\n", result);
            return;
        }
    }
		//加密
		uint16_t voltage_int = (uint16_t)voltage;
		uint16_t voltage_frac = (uint16_t)((voltage - voltage_int) * 65536);
		uint32_t voltage_hex = (voltage_int << 16) | voltage_frac;
	
	
    // 写入数据
    if(hidedata_file_open) {
        uint32_t timestamp = get_unix_timestamp();
        local_time_t local_time = timestamp_to_local_time(timestamp);
        
        char data_line[256];
        // 格式：2025-01-01 00:30:10 1.5V
        //      hide: XXXXXXXXXXXXXXXX
        sprintf(data_line, "%04d-%02d-%02d %02d:%02d:%02d %.1fV\r\nhide: %08X%08X\r\n",
                local_time.year, local_time.month, local_time.day,
                local_time.hour, local_time.minute, local_time.second,
                voltage, timestamp, (uint32_t)(voltage_hex));
        
        UINT bytes_written;
        result = f_write(&current_hidedata_file, data_line, strlen(data_line), &bytes_written);
        if(result == FR_OK && bytes_written == strlen(data_line)) {
            f_sync(&current_hidedata_file);
            storage_state.hidedata_count++;
        } else {
            // my_printf(DEBUG_USART, "HideData write error: %d, bytes: %d/%d\r\n", 
            //          result, bytes_written, strlen(data_line));
        }
    }
}

// 保存配置到TF卡config.ini文件的函数
void save_config_to_file(void)
{
    FIL config_file;
    FRESULT result;
    char write_buffer[256];
    UINT bytes_written;
    
    // 创建或打开config.ini文件进行写入
    result = f_open(&config_file, "/config.ini", FA_CREATE_ALWAYS | FA_WRITE);
    if(result != FR_OK) {
//        my_printf(DEBUG_USART, "Failed to create config.ini file: %d\r\n", result);
        return;
    }
    
    // 构建配置文件内容
    sprintf(write_buffer, "[Ratio]\r\nCh0=");
    result = f_write(&config_file, write_buffer, strlen(write_buffer), &bytes_written);
    
    sprintf(write_buffer, "%.1f\r\n\r\n[Limit]\r\nCh0=", system_config.ratio_ch0);
    result = f_write(&config_file, write_buffer, strlen(write_buffer), &bytes_written);
    
    sprintf(write_buffer, "%.2f\r\n", system_config.limit_ch0);
    result = f_write(&config_file, write_buffer, strlen(write_buffer), &bytes_written);
    
    // 关闭文件
    f_close(&config_file);
}



