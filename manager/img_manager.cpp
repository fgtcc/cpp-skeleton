#include "img_manager.h"

namespace fgtcc {
    
namespace imgproc {

//imgType 包括png bmp jpg jpeg等opencv能够进行编码解码的文件
std::string Mat2Base64(const cv::Mat &img, std::string imgType) {
	//Mat转base64
	std::string img_data;
	std::vector<uchar> vecImg;
	std::vector<int> vecCompression_params;
	vecCompression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
	vecCompression_params.push_back(90);
	imgType = "." + imgType;
	cv::imencode(imgType, img, vecImg, vecCompression_params);
	img_data = fgtcc::Base64Encode(vecImg.data(), vecImg.size());
	return img_data;
}

cv::Mat Base2Mat(std::string &base64_data) {
	cv::Mat img;
	std::string s_mat;
	s_mat = fgtcc::Base64Decode(base64_data);
	std::vector<char> base64_img(s_mat.begin(), s_mat.end());
	img = cv::imdecode(base64_img, cv::IMREAD_UNCHANGED);
	return img;
}

std::string GetOpencvVersion() {
	da4qi4::log::Server()->info("get OpenCV version");
	std::string version = CV_VERSION;
	return version;
}

} // namespace imgproc

} // namespace fgtcc