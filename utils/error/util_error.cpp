#include "util_error.h"

namespace fgtcc{

std::string error_str (ErrorCode code)
{
    std::string msg = "success";

    switch (code) {
        case ERR_SUCCESS: 
            msg = "success";
            break;
        case ERR_ERROR:
            msg = "fail";
            break;
        case ERR_IO_ERROR:
            msg = "io error";
            break;
        case ERR_STORAGE_ERROR:
            msg = "storage error";
            break;
        case ERR_SYS_ERROR:
            msg = "system error";
            break;
        case ERR_FILE_ERROR:
            msg = "file error";
            break;
        case ERR_JSON_ERROR:
            msg = "json error";
            break;
        case ERR_SOCKET_ERROR:
            msg = "socket error";
            break;
        case ERR_CONFIG_ERROR:
            msg = "config error";
            break;
        case ERR_MEMORY_ERROR:
            msg = "memory error";
            break;
        case ERR_INVALID_PARAM:
            msg = "invalid param";
            break;
        case ERR_INVALID_PATH:
            msg = "invalid path";
            break;
        case ERR_INVALID_FORMAT:
            msg = "invalid format";
            break;
        case ERR_INVALID_TIMESTAMP:
            msg = "invalid timestamp";
            break;
        case ERR_INVALID_COMMAND:
            msg = "invalid command code";
            break;
        case ERR_INVALID_DEV_TYPE:
            msg = "invalid device type";
            break;
        default:
            msg = "fail";
            break;
    }

    return msg;
}

}