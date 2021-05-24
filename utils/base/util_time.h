#ifndef __UTIL_TIME_H__
#define __UTIL_TIME_H__

#include <stdio.h> 
#include <memory.h>
#include <ctime>
#include <string>

namespace fgtcc {

time_t strTime2unix(std::string timeStamp);
std::string timestamp2str(time_t timestamp);

} // namespace fgtcc

#endif /* __UTIL_TIME_H__ */