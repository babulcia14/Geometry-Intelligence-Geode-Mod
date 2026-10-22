#pragma once
#include "Api.hpp"
namespace gi {
class Engine {
    Http& http;
    Api& api;
    Config config;
    std::string key;
    Json complete(Json const& messages,bool tools,std::stop_token,Stream const&);
public:
    Engine(Http& h,Api& a,Config c,std::string token):http(h),api(a),config(std::move(c)),key(std::move(token)){}
    Answer ask(std::vector<Message> const& history,Json const& gameContext,std::stop_token,Progress const&,Stream const& = {});
    static Json contextMessages(std::vector<Message> const&,Config const&,Json const& game);
};
}
