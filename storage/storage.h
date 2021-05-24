#ifndef __STORAGE_H__
#define __STORAGE_H__

#include "daqi/da4qi4.hpp"
#include "typedef.h"

namespace fgtcc {

class ConfigStorage {
private:
    //私有构造函数，不允许使用者自己生成对象
    ConfigStorage() {
        da4qi4::Json cfg;
        std::ifstream cfgFile("config/config.json");
        cfgFile >> cfg;
        svcCfg = cfg.at("service").get<fgtcc::SvcConfig>();
    }
    ConfigStorage(const ConfigStorage& other) {}

public:
    // 注意返回的是引用。
    static ConfigStorage& getInstance() {
        static ConfigStorage m_instance;
        return m_instance;
    }

public:
    fgtcc::SvcConfig svcCfg;
};

} // namespace fgtcc

#endif /* __STORAGE_H__ */