#ifndef __img_SERVICE_H__
#define __img_SERVICE_H__

#include "img_manager.h"
#include "typedef.h"

namespace fgtcc {

namespace imgsvc {
    std::string GetOpencvVersion(fgtcc::Error &err);
    void GetImageInfo(std::string imgBase64, fgtcc::ImageInfo& info, fgtcc::Error &err);
} // namespace imgsvc

} // namespace fgtcc

#endif /* __img_SERVICE_H__ */
