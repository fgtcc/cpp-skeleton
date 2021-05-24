#ifndef __TYPEDEF_H__
#define __TYPEDEF_H__

#include "serialization.h"
#include <iostream>
#include <string>
#include <vector>

namespace fgtcc {

struct SvcConfig {
    int port;
};

struct ImageInfo {
    int width;
    int height;
};

struct GetOpencvVersionResp {
    std::string version;
};

struct GetImageInfoReq {
    std::string imgBase64;
};

struct GetImageInfoResp {
    int width;
    int height;
};

} // namespace fgtcc

DEFINE_STRUCT_SCHEMA(fgtcc::SvcConfig,
                    DEFINE_STRUCT_FIELD(port, "port"));

DEFINE_STRUCT_SCHEMA(fgtcc::GetOpencvVersionResp,
                    DEFINE_STRUCT_FIELD(version, "version"));

DEFINE_STRUCT_SCHEMA(fgtcc::GetImageInfoReq,
                    DEFINE_STRUCT_FIELD(imgBase64, "img_base64"));

DEFINE_STRUCT_SCHEMA(fgtcc::GetImageInfoResp,
                    DEFINE_STRUCT_FIELD(width, "width"),
                    DEFINE_STRUCT_FIELD(height, "height"));

#endif /* __TYPEDEF_H__ */