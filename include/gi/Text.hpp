#pragma once
#include "Types.hpp"
#include <string_view>
namespace gi {
std::string trim(std::string s);
std::string lower(std::string s);
std::string encode(std::string_view s);
std::string decode(std::string_view s);
std::string base64Decode(std::string_view s);
std::vector<std::string> split(std::string_view s, std::string_view delimiter);
std::string utcNow();
int64_t epochNow();
std::string newId();
std::string clip(std::string s, size_t maxBytes);
std::string plainText(std::string s);
int64_t number(std::string const& s, int64_t fallback = 0);
}
