#ifndef __UTIL_ERROR_H__
#define __UTIL_ERROR_H__

#include "string"
#include "error_const.h"

namespace fgtcc {

std::string error_str (ErrorCode code);

class Error {
public:
   Error(ErrorCode code) : _code (code){ _msg = error_str(code);}
   Error(ErrorCode code, std::string msg) : _code (code), _msg(msg) {}
   Error() : _code(0), _msg("success") {}
   void SetCode(ErrorCode code){_code = code; _msg = error_str(code);}
   void SetCode(ErrorCode code, std::string msg){_code = code; _msg = msg;}

   std::string String() { return _msg;}
   ErrorCode Code() { return _code;} 

private:
   int _code;
   std::string _msg;
}; 

} // namespace fgtcc

#endif /* __UTIL_ERROR_H__ */