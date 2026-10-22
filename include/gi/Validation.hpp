#pragma once
#include "Types.hpp"
namespace gi {
struct Url { std::string scheme, host, path, origin; int port; bool loopback; };
Url parseUrl(std::string const& url);
std::string validateEndpoint(std::string endpoint, bool remoteConsent);
bool safeSourceUrl(std::string const& url);
Json parseJson(std::string const& body);
std::string requiredString(Json const& j, char const* key, size_t max=256);
int boundedInt(Json const& j, char const* key, int fallback, int low, int high);
void validateConfig(Config const& c);
}
