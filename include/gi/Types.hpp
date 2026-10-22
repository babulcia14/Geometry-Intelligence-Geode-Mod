#pragma once
#include <nlohmann/json.hpp>
#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <vector>

namespace gi {
using Json = nlohmann::json;
enum class ErrorCode { Cancelled, Timeout, Network, Http, RateLimit, InvalidData,
    Configuration, Credentials, Storage, Model, Consent, Limit };
class Error final : public std::runtime_error {
public:
    ErrorCode code;
    Error(ErrorCode c, std::string message) : std::runtime_error(std::move(message)), code(c) {}
};
inline void checkCancelled(std::stop_token token) {
    if (token.stop_requested()) throw Error(ErrorCode::Cancelled, "Cancelled / Anulowano.");
}
struct Config {
    std::string endpoint = "http://127.0.0.1:18765/v1";
    std::string model = "gi-tiny";
    std::string language = "pl";
    std::string gdVersion = "2.2081";
    std::string geodeVersion = "5.11.2";
    bool allowRemote = false;
    bool allowPublicApis = false;
    bool allowContext = false;
    bool saveHistory = false;
    bool cacheEnabled = true;
    bool streaming = true;
    bool pointercrateEnabled = true;
    bool geodeEnabled = true;
    bool musicEnabled = true;
    bool webEnabled = true;
    bool levelsEnabled = true;
    int cacheSeconds = 120;
    int timeoutSeconds = 90;
    int maxTokens = 600;
};
struct Source {
    std::string title, url, fetchedAt, confidence = "primary";
    bool cached = false;
    Json data;
};
struct Message {
    std::string role, content, time;
    std::vector<Source> sources;
};
struct Conversation {
    std::string id, title;
    std::vector<Message> messages;
};
struct Answer {
    std::string text;
    std::vector<Source> sources;
};
using Progress = std::function<void(std::string)>;
using Stream = std::function<void(std::string const&)>;
void to_json(Json& j, Source const& s);
void from_json(Json const& j, Source& s);
void to_json(Json& j, Message const& m);
void from_json(Json const& j, Message& m);
void to_json(Json& j, Conversation const& c);
void from_json(Json const& j, Conversation& c);
}
