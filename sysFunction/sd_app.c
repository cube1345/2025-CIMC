#include "ir_cmic_gd32f470vet6.h"
#include "usart_app.h"
#include "ff.h"
#include "sd_app.h"

// ԭ�е�SD�����Ա���
FIL fdst;
uint16_t i = 0, count, result = 0;
UINT br, bw;
sd_card_info_struct sd_cardinfo;
BYTE buffer[128];
BYTE filebuffer[128];

// �ļ�ϵͳ�ʹ洢���ȫ�ֱ���
FATFS fs;
extern config_params_t system_config;
extern storage_state_t storage_state;

// �ļ��������
FIL current_log_file;
FIL current_sample_file;
FIL current_overlimit_file;
FIL current_hidedata_file;

// �ļ��򿪱�־
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
    nvic_irq_enable(SDIO_IRQn, 0, 0);					// ʹ��SDIO�жϣ����ȼ�Ϊ0
}

/**
 * @brief       ͨ�����ڴ�ӡSD�������Ϣ
 * @param       ��
 * @retval      ��
 */
void card_info_get(void)
{
    sd_card_info_struct sd_cardinfo;      // SD����Ϣ�ṹ��
    sd_error_enum status;                 // SD������״̬
    uint32_t block_count, block_size;
    
    // ��ȡSD����Ϣ
    status = sd_card_information_get(&sd_cardinfo);
    
    if(SD_OK == status)
    {
        my_printf(DEBUG_USART, "\r\n*** SD Card Info ***\r\n");
        
        // ��ӡ������
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
        
        // ��ӡ�������Ϳ��С
        block_count = (sd_cardinfo.card_csd.c_size + 1) * 1024;
        block_size = 512;
        my_printf(DEBUG_USART,"\r\n## Device size is %dKB (%.2fGB)##", sd_card_capacity_get(), sd_card_capacity_get() / 1024.0f / 1024.0f);
        my_printf(DEBUG_USART,"\r\n## Block size is %dB ##", block_size);
        my_printf(DEBUG_USART,"\r\n## Block count is %d ##", block_count);
        
        // ��ӡ������ID�Ͳ�Ʒ����
        my_printf(DEBUG_USART, "Manufacturer ID: 0x%X\r\n", sd_cardinfo.card_cid.mid);
        my_printf(DEBUG_USART, "OEM/Application ID: 0x%X\r\n", sd_cardinfo.card_cid.oid);
        
        // ��ӡ��Ʒ���� (PNM)
        uint8_t pnm[6];
        pnm[0] = (sd_cardinfo.card_cid.pnm0 >> 24) & 0xFF;
        pnm[1] = (sd_cardinfo.card_cid.pnm0 >> 16) & 0xFF;
        pnm[2] = (sd_cardinfo.card_cid.pnm0 >> 8) & 0xFF;
        pnm[3] = sd_cardinfo.card_cid.pnm0 & 0xFF;
        pnm[4] = sd_cardinfo.card_cid.pnm1 & 0xFF;
        pnm[5] = '\0';
        my_printf(DEBUG_USART, "Product Name: %s\r\n", pnm);
        
        // ��ӡ��Ʒ�汾�����к�
        my_printf(DEBUG_USART, "Product Revision: %d.%d\r\n", (sd_cardinfo.card_cid.prv >> 4) & 0x0F, sd_cardinfo.card_cid.prv & 0x0F);
        // ���к����޷��ŷ�ʽ��ʾ�����⸺��
        my_printf(DEBUG_USART, "Product Serial Number: 0x%08X\r\n", sd_cardinfo.card_cid.psn);
        
        // ��ӡCSD�汾������CSD��Ϣ
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
    
    // ���Դ�config.ini�ļ���ʹ��ȫ�ֹ��ص��ļ�ϵͳ��
    result = f_open(&config_file, "/config.ini", FA_READ);
    if(result != FR_OK) {
        my_printf(DEBUG_USART, "config.ini file not found.\r\n");
        return;
    }
  
    // ���������ļ�
    uint8_t ratio_section_found = 0;
    uint8_t limit_section_found = 0;
    uint8_t ratio_parsed = 0;
    uint8_t limit_parsed = 0;
    
    while(f_gets(line_buffer, sizeof(line_buffer), &config_file) != NULL) {
        // �Ƴ���ĩ�Ļ��з�
        char* newline = strchr(line_buffer, '\n');
        if(newline) *newline = '\0';
        char* carriage = strchr(line_buffer, '\r');
        if(carriage) *carriage = '\0';
        
        // �������к�ע��
        if(strlen(line_buffer) == 0 || line_buffer[0] == ';' || line_buffer[0] == '#') {
            continue;
        }
        
        // ����Ƿ��ǽڱ���
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
        // ����������
        else if(ratio_section_found && strstr(line_buffer, "Ch0") != NULL) {
            char* equal_sign = strchr(line_buffer, '=');
            if(equal_sign != NULL) {
                equal_sign++; // �����Ⱥ�
                while(*equal_sign == ' ') equal_sign++; // �����ո�
                system_config.ratio_ch0 = atof(equal_sign);
                ratio_parsed = 1;
            }
        }
        else if(limit_section_found && strstr(line_buffer, "Ch0") != NULL) {
            char* equal_sign = strchr(line_buffer, '=');
            if(equal_sign != NULL) {
                equal_sign++; // �����Ⱥ�
                while(*equal_sign == ' ') equal_sign++; // �����ո�
                system_config.limit_ch0 = atof(equal_sign);
                limit_parsed = 1;
            }
        }
    }
    
    // �ر��ļ�
    f_close(&config_file);
    
    // ����Ƿ�ɹ����������б�Ҫ�Ĳ���
    if(ratio_parsed && limit_parsed) {
        // �����ȡ������ - �ֶ�����������ʾ��������λС��
        // int ratio_int = (int)system_config.ratio_ch0;
        // int ratio_frac = (int)((system_config.ratio_ch0 - ratio_int) * 100);
        // my_printf(DEBUG_USART, "Ratio = %d.%02d\r\n", ratio_int, ratio_frac);
        
        // int limit_int = (int)system_config.limit_ch0;
        // int limit_frac = (int)((system_config.limit_ch0 - limit_int) * 100);
        // my_printf(DEBUG_USART, "Limit = %d.%02d\r\n", limit_int, limit_frac);
        my_printf(DEBUG_USART, "ratio: %.1f\r\n", system_config.ratio_ch0);
        my_printf(DEBUG_USART, "limit: %.2f\r\n", system_config.limit_ch0);
        
        // ������д��Flash (����򻯴���ʵ��Ӧ������Ҫѡ����ʵ�Flash��ַ)
        if(write_config_to_flash(&system_config) == SUCCESS) {
            my_printf(DEBUG_USART, "config read success\r\n");
        } else {
//            my_printf(DEBUG_USART, "Failed to write config to flash\r\n");
        }
    } else {
//        my_printf(DEBUG_USART, "Invalid config file format\r\n");
    }
}

// =================== ���ݴ洢��غ��� ===================

// ��ȡ��ǰʱ�������ʱ���ַ�������ʽ��YYYYMMDDHHMMSS��
void get_datetime_string(char* datetime_str)
{
    uint32_t timestamp = get_unix_timestamp();
    local_time_t local_time = timestamp_to_local_time(timestamp);
    
    sprintf(datetime_str, "%04d%02d%02d%02d%02d%02d",
            local_time.year, local_time.month, local_time.day,
            local_time.hour, local_time.minute, local_time.second);
}

// �����洢Ŀ¼
void create_storage_directories(void)
{
    FRESULT result;

    // ��Ĭ�����洢�ļ���
    f_mkdir("sample");
    f_mkdir("overLimit");
    f_mkdir("log");
    f_mkdir("hideData");

    // ��Ĭģʽ��������κ���Ϣ
}

// ��Flash��ȡ��־ID
uint32_t get_next_log_id(void)
{
    uint32_t log_id_flash_addr = 0x2000; // ʹ�ò�ͬ��Flash��ַ�洢��־ID
    uint32_t stored_log_id;
    
    spi_flash_buffer_read((uint8_t*)&stored_log_id, log_id_flash_addr, sizeof(uint32_t));
    
    // ����ȡ��ֵ�Ƿ���Ч
    if(stored_log_id == 0xFFFFFFFF || stored_log_id > 10000) {
        stored_log_id = 0; // �״�ʹ�û�������Ч
    }
    
    return stored_log_id;
}

// ������־ID��Flash
void save_log_id_to_flash(uint32_t log_id)
{
    uint32_t log_id_flash_addr = 0x2000;
    
    // ��������
    spi_flash_sector_erase(log_id_flash_addr);
    spi_flash_wait_for_write_end();
    
    // д����־ID
    spi_flash_buffer_write((uint8_t*)&log_id, log_id_flash_addr, sizeof(uint32_t));
}

// �洢ϵͳ��ʼ��
void storage_init(void)
{
    FRESULT result;
    
    // ȷ��SD���Ѿ���ʼ��
    DSTATUS sd_status = disk_initialize(0);
    if(sd_status != RES_OK) {
        return;
    }
    
    // �����ļ�ϵͳ�����ֹ���״̬
    result = f_mount(0, &fs);
    if(result != FR_OK) {
//        my_printf(DEBUG_USART, "Failed to mount TF card for storage init: %d\r\n", result);
        return;
    }

    // �����洢Ŀ¼ (��Ĭģʽ)
    create_storage_directories();

    // ��ȡ��־ID
    storage_state.log_id = get_next_log_id();

    // ������־�ļ� - ʹ��log�ļ���
    char log_filename[64];
    sprintf(log_filename, "log/log%u.txt", storage_state.log_id);

    result = f_open(&current_log_file, log_filename, FA_CREATE_ALWAYS | FA_WRITE);
    if(result == FR_OK) {
        log_file_open = 1;
    } else {
    }
    
    // �����������־ID��Flash
    save_log_id_to_flash(storage_state.log_id + 1);
}

// ��¼������־
void log_operation(const char* operation)
{
    
    if(!log_file_open) return;
    
    uint32_t timestamp = get_unix_timestamp();
    local_time_t local_time = timestamp_to_local_time(timestamp);
    
    char log_entry[256];
    // ��ʽ��2025-01-01 10:00:01 system init
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

// �洢��������
void store_sample_data(float voltage, uint8_t over_limit)
{
    // ��������˼��ܴ洢����ͨ�������ݲ��洢��sample�ļ���
    if(storage_state.hide_storage_enabled) {
        store_hidedata(voltage, over_limit);
        return;
    }
    
    FRESULT result;
    
	
	
	
    // �����Ҫ�½��ļ����ļ�δ��
    if(!sample_file_open || storage_state.sample_count >= 10) {
        // �رյ�ǰ�ļ�
        if(sample_file_open) {
            f_close(&current_sample_file);
            sample_file_open = 0;
        }
        
        // �������ļ� - ʹ��sample�ļ��к���ȷ���ļ�����ʽ
        char datetime_str[16];
        get_datetime_string(datetime_str);
        char filename[64];
        sprintf(filename, "sample/sampleData%s.txt", datetime_str);
        
        result = f_open(&current_sample_file, filename, FA_CREATE_ALWAYS | FA_WRITE);
        if(result == FR_OK) {
            sample_file_open = 1;
            storage_state.sample_count = 0;
            
            // sample�ļ�����Ҫ�ļ�ͷ��ֱ��д������
            
            // my_printf(DEBUG_USART, "Sample file created: %s\r\n", filename);
        } else {
            // my_printf(DEBUG_USART, "Failed to create sample file, error: %d\r\n", result);
            return;
        }
    }
    
    // д������
    if(sample_file_open) {
        uint32_t timestamp = get_unix_timestamp();
        local_time_t local_time = timestamp_to_local_time(timestamp);
        
        char data_line[128];
        // ��ʽ��2025-01-01 00:30:10 1.5V
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

// �洢����ֵ����
void store_overlimit_data(float voltage)
{
    FRESULT result;
    
    // �����Ҫ�½��ļ����ļ�δ��
    if(!overlimit_file_open || storage_state.overlimit_count >= 10) {
        // �رյ�ǰ�ļ�
        if(overlimit_file_open) {
            f_close(&current_overlimit_file);
            overlimit_file_open = 0;
        }
        
        // �������ļ� - ʹ��overLimit�ļ��к���ȷ���ļ�����ʽ
        char datetime_str[16];
        get_datetime_string(datetime_str);
        char filename[64];
        sprintf(filename, "overLimit/overLimit%s.txt", datetime_str);
        
        result = f_open(&current_overlimit_file, filename, FA_CREATE_ALWAYS | FA_WRITE);
        if(result == FR_OK) {
            overlimit_file_open = 1;
            storage_state.overlimit_count = 0;
            
            // overLimit�ļ�����Ҫ�ļ�ͷ��ֱ��д������
            
            // my_printf(DEBUG_USART, "OverLimit file created: %s\r\n", filename);
        } else {
            // my_printf(DEBUG_USART, "Failed to create overlimit file, error: %d\r\n", result);
            return;
        }
    }
    
    // д������
    if(overlimit_file_open) {
        uint32_t timestamp = get_unix_timestamp();
        local_time_t local_time = timestamp_to_local_time(timestamp);
        
        char data_line[128];
        // ��ʽ��2025-01-01 00:30:10 30V limit 10V
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

// �洢��������
void store_hidedata(float voltage, uint8_t over_limit)
{
    FRESULT result;
    
    // �����Ҫ�½��ļ����ļ�δ��
    if(!hidedata_file_open || storage_state.hidedata_count >= 10) {
        // �رյ�ǰ�ļ�
        if(hidedata_file_open) {
            f_close(&current_hidedata_file);
            hidedata_file_open = 0;
        }
        
        // �������ļ� - ʹ��hideData�ļ��к���ȷ���ļ�����ʽ
        char datetime_str[16];
        get_datetime_string(datetime_str);
        char filename[64];
        sprintf(filename, "hideData/hideData%s.txt", datetime_str);
        
        result = f_open(&current_hidedata_file, filename, FA_CREATE_ALWAYS | FA_WRITE);
        if(result == FR_OK) {
            hidedata_file_open = 1;
            storage_state.hidedata_count = 0;
            
            // hideData�ļ�����Ҫ�ļ�ͷ��ֱ��д������
            
            // ��Ĭģʽ������ʾhideData�ļ�������Ϣ
        } else {
            // my_printf(DEBUG_USART, "Failed to create hidedata file, error: %d\r\n", result);
            return;
        }
    }
		//����
		uint16_t voltage_int = (uint16_t)voltage;
		uint16_t voltage_frac = (uint16_t)((voltage - voltage_int) * 65536);
		uint32_t voltage_hex = (voltage_int << 16) | voltage_frac;
	
	
    // д������
    if(hidedata_file_open) {
        uint32_t timestamp = get_unix_timestamp();
        local_time_t local_time = timestamp_to_local_time(timestamp);
        
        char data_line[256];
        // ��ʽ��2025-01-01 00:30:10 1.5V
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

// �������õ�TF��config.ini�ļ��ĺ���
void save_config_to_file(void)
{
    FIL config_file;
    FRESULT result;
    char write_buffer[256];
    UINT bytes_written;
    
    // �������config.ini�ļ�����д��
    result = f_open(&config_file, "/config.ini", FA_CREATE_ALWAYS | FA_WRITE);
    if(result != FR_OK) {
//        my_printf(DEBUG_USART, "Failed to create config.ini file: %d\r\n", result);
        return;
    }
    
    // ���������ļ�����
    sprintf(write_buffer, "[Ratio]\r\nCh0=");
    result = f_write(&config_file, write_buffer, strlen(write_buffer), &bytes_written);
    
    sprintf(write_buffer, "%.1f\r\n\r\n[Limit]\r\nCh0=", system_config.ratio_ch0);
    result = f_write(&config_file, write_buffer, strlen(write_buffer), &bytes_written);
    
    sprintf(write_buffer, "%.2f\r\n", system_config.limit_ch0);
    result = f_write(&config_file, write_buffer, strlen(write_buffer), &bytes_written);
    
    // �ر��ļ�
    f_close(&config_file);
}



