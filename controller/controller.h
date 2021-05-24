#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#include "img_service.h"
#include "typedef.h"

namespace fgtcc {

namespace imgctrl {
    void GetOpencvVersion(da4qi4::Context ctx);
    void GetImageInfo(da4qi4::Context ctx);
} // namespace imgctrl

    da4qi4::Json assembleResponse(da4qi4::Json resp, fgtcc::Error& err);
    da4qi4::Json assembleResponse(fgtcc::Error& err);

} // namespace fgtcc

#endif