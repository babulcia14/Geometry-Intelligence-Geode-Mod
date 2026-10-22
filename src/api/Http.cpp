#include <gi/Http.hpp>
#include <gi/Text.hpp>
#include <gi/Validation.hpp>
#include <thread>
namespace gi {
Response Http::request(Request const& req,std::stop_token stop){
    checkCancelled(stop);
    auto u=parseUrl(req.url);auto origin=u.origin;
    // Shared across turns/Http instances. MusicBrainz permits roughly 1 req/s.
    static std::mutex guard;
    static std::map<std::string,int64_t> blocked;
    static std::map<std::string,std::chrono::steady_clock::time_point> next;
    std::chrono::steady_clock::time_point due;
    {std::lock_guard lock(guard);
        if(blocked[origin]>epochNow())throw Error(ErrorCode::RateLimit,"Rate limit: wait "+std::to_string(blocked[origin]-epochNow())+" s before retrying.");
        due=std::max(next[origin],std::chrono::steady_clock::now());
        next[origin]=due+std::chrono::milliseconds(u.loopback?0:u.host=="musicbrainz.org"?1100:250);
    }
    while(std::chrono::steady_clock::now()<due){checkCancelled(stop);std::this_thread::sleep_for(std::chrono::milliseconds(20));}
    auto r=transport.perform(req,stop);checkCancelled(stop);
    if(r.body.size()>req.maxBytes)throw Error(ErrorCode::Limit,"Response is too large.");
    if(r.status==429||(u.host=="musicbrainz.org"&&r.status==503)){auto seconds=number(r.headers["retry-after"],60);{std::lock_guard lock(guard);blocked[origin]=epochNow()+std::clamp<int64_t>(seconds,1,3600);}throw Error(ErrorCode::RateLimit,"HTTP rate limit/service busy. Retry later; no automatic repeated requests.");}
    if(r.status<200||r.status>=300)throw Error(ErrorCode::Http,"HTTP "+std::to_string(r.status)+" from "+parseUrl(req.url).host+".");
    return r;
}
Json Http::json(Request const& r,std::stop_token t){return parseJson(request(r,t).body);}
}
