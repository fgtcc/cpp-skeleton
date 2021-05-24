#include "img_service.h"

namespace fgtcc {

namespace imgsvc {

std::string GetOpencvVersion(fgtcc::Error &err) {
    da4qi4::log::Server()->info("get OpenCV version");
    return imgproc::GetOpencvVersion();
}

void GetImageSizeInfo(std::string imgBase64, fgtcc::ImageSizeInfo& info, fgtcc::Error &err) {
    da4qi4::log::Server()->info("get image size info");
    cv::Mat img = imgproc::Base2Mat(imgBase64);
    if (!img.data) {
        err.SetCode(fgtcc::ERR_ERROR);
        da4qi4::log::Server()->error("invalid img data");
        return;
    }

    info.width = img.cols;
    info.height = img.rows;
    return;
}

} // namespace imgsvc

} // namespace fgtcc