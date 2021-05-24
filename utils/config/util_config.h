/*
    https://github.com/ReneNyffenegger/cpp-read-configuration-files
*/

#ifndef __UTIL_CONFIG_H__
#define __UTIL_CONFIG_H__

#include <string>
#include <map>
#include "chameleon.h"

namespace fgtcc{
    
class ConfigFile {
  std::map<std::string,Chameleon> content_;

public:
  ConfigFile(std::string const& configFile);

  Chameleon const& Value(std::string const& section, std::string const& entry) const;

  Chameleon const& Value(std::string const& section, std::string const& entry, double value);
  Chameleon const& Value(std::string const& section, std::string const& entry, std::string const& value);
};

} // namespace fgtcc


#endif /* __UTIL_CONFIG_H__ */
