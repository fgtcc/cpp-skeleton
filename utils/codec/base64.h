#ifndef _BASE64_H_
#define _BASE64_H_

#include <cstdint>
#include <string>

namespace fgtcc {

std::string Base64Encode(const std::uint8_t* data, std::size_t length);

std::string Base64Encode(const std::string& input);

std::string Base64Decode(const std::string& input);

}  // namespace fgtcc

#endif//_BASE64_H_