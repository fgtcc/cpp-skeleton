#include "controller.h"

namespace fgtcc {

namespace imgctrl {

void GetOpencvVersion(da4qi4::Context ctx) {
    fgtcc::Error err;
    std::string version = fgtcc::imgsvc::GetOpencvVersion(err);
    fgtcc::GetOpencvVersionResp resp = {
        version: version
    };

    da4qi4::Json res = assembleResponse(da4qi4::Json(resp), err);
    ctx->Res().ReplyOk(res);
    ctx->Pass();
}

void GetImageInfo(da4qi4::Context ctx) {
    auto reqBody = ctx->Req().Body();
    // da4qi4::log::Server()->info("Detect req: {}", reqBody);
    auto jsonData = da4qi4::Json::parse(reqBody);

    fgtcc::Error err;
    fgtcc::GetImageInfoReq req;
    try {
        req = jsonData.get<fgtcc::GetImageInfoReq>();
    } catch (std::exception& e) {
        err.SetCode(fgtcc::ERR_INVALID_PARAM, e.what());
        da4qi4::log::Server()->error("invalid param when get img info, err: {}", e.what());
        da4qi4::Json res = assembleResponse(err);
        ctx->Res().ReplyOk(res);
        ctx->Pass();
        return;
    }

    fgtcc::ImageInfo info;
    fgtcc::imgsvc::GetImageInfo(req.imgBase64, &info, err);
    fgtcc::GetImageInfoResp resp = {
        width: info.width,
        height: info.height
    };

    da4qi4::Json res = assembleResponse(da4qi4::Json(resp), err);
    ctx->Res().ReplyOk(res);
    ctx->Pass();
}

} // namespace imgctrl

da4qi4::Json assembleResponse(da4qi4::Json resp, fgtcc::Error& err) {
    da4qi4::Json res;
    if (err.Code() != fgtcc::ERR_SUCCESS) {
        res["result"] = da4qi4::Json::object();
    } else {
        res["result"] = resp;
    }

    res["code"] = err.Code();
    res["message"] = err.String();
    return res;
}

da4qi4::Json assembleResponse(fgtcc::Error& err) {
    da4qi4::Json res;
    res["code"] = err.Code();
    res["message"] = err.String();
    res["result"] = da4qi4::Json::object();
    return res;
}

} // namespace fgtcc