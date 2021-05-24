#include "controller.h"
#include "storage.h"

int main()
{
    da4qi4::log::InitServerLogger("./log/", da4qi4::log::Level::debug);

    fgtcc::SvcConfig svcCfg = fgtcc::ConfigStorage::getInstance().svcCfg;
    auto svc = da4qi4::Server::Supply(svcCfg.port);
    da4qi4::log::Server()->info("serving is running at port: {}", svcCfg.port);

    svc->AddHandler(da4qi4::_POST_, "/api/getOpencvVersion", fgtcc::imgctrl::GetOpencvVersion);
    svc->AddHandler(da4qi4::_POST_, "/api/GetImageSizeInfo", fgtcc::imgctrl::GetImageSizeInfo);

    svc->Run();
    da4qi4::log::Server()->info("goodbye!!!");
}
