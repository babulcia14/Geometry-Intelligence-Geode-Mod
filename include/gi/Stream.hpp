#pragma once
#include "Types.hpp"
#include <string_view>

namespace gi {
// Incremental Chat Completions SSE parser. A network chunk need not end at a
// UTF-8 character, JSON boundary or even a complete SSE line.
class ChatStream {
    std::string pending;
    std::string content;
    std::map<int, Json> calls;
    std::string reason;
    bool done = false;
    bool finished = false;
    size_t bytes = 0;
    Stream onText;
    void line(std::string const&);
public:
    explicit ChatStream(Stream callback = {}) : onText(std::move(callback)) {}
    void feed(std::string_view);
    Json response();
};
}
