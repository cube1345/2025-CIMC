#include "ir_cmic_gd32f470vet6.h"

extern rtc_parameter_struct rtc_initpara;


uint8_t bcd_to_dec(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

uint8_t dec_to_bcd(uint8_t dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}

uint8_t is_leap_year(uint16_t year)
{
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

uint8_t get_days_in_month(uint8_t month, uint16_t year)
{
    const uint8_t days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    if (month == 2 && is_leap_year(year)) {
        return 29;
    }
    return days_in_month[month - 1];
}

#define TIMEZONE_OFFSET_HOURS 8
#define TIMEZONE_OFFSET_SECONDS (TIMEZONE_OFFSET_HOURS * 3600)

local_time_t timestamp_to_local_time(uint32_t timestamp)
{
    local_time_t local_time = {0};
    
    uint32_t local_timestamp = timestamp + TIMEZONE_OFFSET_SECONDS;
    
    uint32_t days_since_epoch = local_timestamp / 86400;
    uint32_t seconds_in_day = local_timestamp % 86400;
    
    uint16_t year = 1970;
    uint32_t remaining_days = days_since_epoch;
    uint32_t days_in_year;
    
    while (remaining_days >= (days_in_year = (is_leap_year(year) ? 366 : 365))) {
        remaining_days -= days_in_year;
        year++;
    }
    
    uint8_t month = 1;
    uint32_t days_in_month_val;
    
    while (remaining_days >= (days_in_month_val = get_days_in_month(month, year))) {
        remaining_days -= days_in_month_val;
        month++;
    }
    
    uint8_t day = remaining_days + 1;
    
    uint8_t hour = seconds_in_day / 3600;
    uint8_t minute = (seconds_in_day % 3600) / 60;
    uint8_t second = seconds_in_day % 60;
    
    local_time.year = year;
    local_time.month = month;
    local_time.day = day;
    local_time.hour = hour;
    local_time.minute = minute;
    local_time.second = second;
    
    return local_time;
}

uint32_t get_unix_timestamp(void)
{
    extern rtc_parameter_struct rtc_initpara;
    
    rtc_current_time_get(&rtc_initpara);
    
    uint16_t year = 2000 + bcd_to_dec(rtc_initpara.year);
    uint8_t month = bcd_to_dec(rtc_initpara.month);
    uint8_t day = bcd_to_dec(rtc_initpara.date);
    uint8_t hour = bcd_to_dec(rtc_initpara.hour);
    uint8_t minute = bcd_to_dec(rtc_initpara.minute);
    uint8_t second = bcd_to_dec(rtc_initpara.second);
    
    const uint16_t epoch_year = 1970;
    
    if (year < epoch_year) {
        return 0;
    }
    
    uint32_t days = 0;
    for (uint16_t y = epoch_year; y < year; y++) {
        days += is_leap_year(y) ? 366 : 365;
    }
    
    for (uint8_t m = 1; m < month; m++) {
        days += get_days_in_month(m, year);
    }
    
    days += (day - 1);
    
    uint32_t timestamp = days * 24 * 3600;
    timestamp += hour * 3600;
    timestamp += minute * 60;
    timestamp += second;
    
    return timestamp;
}

/*!
    \brief      display the current time
    \param[in]  none
    \param[out] none
    \retval     none
*/
void rtc_task(void)
{

}
