#pragma once
#include "Http.hpp"
#include "Store.hpp"
#include "Knowledge.hpp"
namespace gi {
Json parseLevels(std::string const& raw);
Json parseSong(std::string const& raw,int requestedId);
class Api {
    Http& http;
    Cache& cache;
    Knowledge& knowledge;
    Config config;
    std::string searchKey;
    Source fetch(std::string const& title,std::string const& url,int ttl,std::stop_token,
        std::function<Json(Json const&)> validate);
    Source demonlist(Json const&,std::stop_token);
    Source mods(Json const&,std::stop_token);
    Source levels(Json const&,std::stop_token);
    Source song(Json const&,std::stop_token);
    Source search(Json const&,std::stop_token);
    Source freeSearch(std::string const& query,std::string const& scope,std::stop_token);
public:
    Api(Http& h,Cache& c,Knowledge& k,Config conf,std::string braveKey={})
        :http(h),cache(c),knowledge(k),config(std::move(conf)),searchKey(std::move(braveKey)){}
    static Json tools();
    std::vector<Source> call(std::string const& name,Json const& args,std::stop_token);
};
}
