#include "util_time.h"

namespace fgtcc {

time_t strTime2unix(std::string timeStamp)  
{  
    struct tm tm;  
    memset(&tm, 0, sizeof(tm));  
      
    sscanf(timeStamp.c_str(), "%d-%d-%d %d:%d:%d",   
           &tm.tm_year, &tm.tm_mon, &tm.tm_mday,  
           &tm.tm_hour, &tm.tm_min, &tm.tm_sec);  
  
    tm.tm_year -= 1900;  
    tm.tm_mon--;  
  
    return mktime(&tm);  
}

std::string timestamp2str(time_t timestamp) {
    struct tm *time;
    time = localtime(&timestamp);

    int year = time->tm_year;   /* 自1900年算起 */
    int month = time->tm_mon;   /* 从1月算起，范围0-11 */
    // int week = time->tm_wday;   /* 从周末算起，范围0-6 */
    // int yday = time->tm_yday;  /* 从1月1日算起，范围0-365 */
    int day = time->tm_mday;    /* 日: 1-31 */
    int hour = time->tm_hour;   /* 小时:0-23点,UTC+0时间 */
    int minute = time->tm_min;  /* 分钟:0-59 */
    int second = time->tm_sec;  /* 0-60，偶尔出现的闰秒 */

    /* 时间校正 */
    year += 1900;
    month += 1;
    // week += 1;

    char timeStr[80];
    sprintf(timeStr, "%d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);

    return std::string(timeStr);
}

} // namespace fgtcc