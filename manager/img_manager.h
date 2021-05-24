#ifndef __img_MANAGER_H__
#define __img_MANAGER_H__

#include <opencv2/opencv.hpp>
#include "daqi/da4qi4.hpp"
#include "util_const.h"
#include "util_error.h"
#include "base64.h"

namespace fgtcc {

namespace imgproc {

std::string Mat2Base64(const cv::Mat &img, std::string imgType);
cv::Mat Base2Mat(std::string &base64_data);

std::string GetOpencvVersion();

} // namespace imgproc

} // namespace fgtcc

#endif /* __img_MANAGER_H__ */