#ifndef __SD_APP_H_
#define __SD_APP_H_

#include "stdint.h"

// SD卡基础功能
void sd_fatfs_init(void);
void sd_fatfs_test(void);
void card_info_get(void);

// 配置文件读取
void config_read_handler(void);

// 存储系统相关函数
void storage_init(void);
void create_storage_directories(void);
uint32_t get_next_log_id(void);
void save_log_id_to_flash(uint32_t log_id);
void get_datetime_string(char* datetime_str);

// 数据存储函数
void store_sample_data(float voltage, uint8_t over_limit);
void store_overlimit_data(float voltage);
void store_hidedata(float voltage, uint8_t over_limit);
void log_operation(const char* operation);

// 配置文件存储
void save_config_to_file(void);

// 文件系统全局变量和文件句柄（外部声明）
extern uint8_t log_file_open;
extern uint8_t sample_file_open;
extern uint8_t overlimit_file_open;
extern uint8_t hidedata_file_open;

#endif /* __SD_APP_H_ */
