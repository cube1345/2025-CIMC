#ifndef __RTC_APP_H_
#define __RTC_APP_H_

#include "stdint.h"
// 原有函数声明
void rtc_task(void);

// 时区相关类型定义
typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} local_time_t;

#define TIMEZONE_OFFSET_HOURS 8
#define TIMEZONE_OFFSET_SECONDS (TIMEZONE_OFFSET_HOURS * 3600)

uint8_t bcd_to_dec(uint8_t bcd);
uint8_t dec_to_bcd(uint8_t dec);
uint8_t is_leap_year(uint16_t year);
uint8_t get_days_in_month(uint8_t month, uint16_t year);

local_time_t timestamp_to_local_time(uint32_t timestamp);
uint32_t get_unix_timestamp(void);


#endif /* __ADC_APP_H_ */
