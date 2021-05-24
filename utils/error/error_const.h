#ifndef __ERROR_CONST_H__
#define __ERROR_CONST_H__

namespace fgtcc {

typedef const int ErrorCode;

////////////////// common error code ///////////////////////
ErrorCode ERR_SUCCESS = 0;
ErrorCode ERR_ERROR = 1;

ErrorCode ERR_IO_ERROR = 1000;
ErrorCode ERR_STORAGE_ERROR = 1001;
ErrorCode ERR_SYS_ERROR = 1002;
ErrorCode ERR_FILE_ERROR = 1003;
ErrorCode ERR_JSON_ERROR = 1004;
ErrorCode ERR_SOCKET_ERROR = 1005;
ErrorCode ERR_CONFIG_ERROR = 1006;
ErrorCode ERR_MEMORY_ERROR = 1007;

ErrorCode ERR_INVALID_PARAM = 1031;
ErrorCode ERR_INVALID_PATH = 1032;
ErrorCode ERR_INVALID_FORMAT = 1033;
ErrorCode ERR_INVALID_TIMESTAMP = 1034;
ErrorCode ERR_INVALID_COMMAND = 1035;
ErrorCode ERR_INVALID_DEV_TYPE = 1036;

} // namespace fgtcc

#endif /* __ERROR_CONST_H__ */