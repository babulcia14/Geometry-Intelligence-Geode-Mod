#include <gi/Validation.hpp>
#include <gi/Text.hpp>
#include <regex>
namespace gi {
Url parseUrl(std::string const& input) {
    if(input.size()>2048||input.find_first_of("\r\n\t \\")!=input.npos)throw Error(ErrorCode::Configuration,"Invalid URL.");
    static const std::regex pattern(R"(^(https?)://([A-Za-z0-9.-]+|\[::1\])(?::([0-9]{1,5}))?(/[^#]*)?$)");
    std::smatch m;
    if(!std::regex_match(input,m,pattern))throw Error(ErrorCode::Configuration,"Use an http(s) URL without credentials, query or fragment in the host.");
    Url u;u.scheme=m[1];u.host=lower(m[2]);u.port=m[3].matched?static_cast<int>(number(m[3],0)):(u.scheme=="https"?443:80);
    if(u.port<1||u.port>65535)throw Error(ErrorCode::Configuration,"Invalid port.");
    u.path=m[4].matched?m[4].str():"/";
    u.origin=u.scheme+"://"+u.host+(m[3].matched?":"+m[3].str():"");
    u.loopback=u.host=="localhost"||u.host=="127.0.0.1"||u.host=="[::1]";
    return u;
}
std::string validateEndpoint(std::string e,bool remoteConsent) {
    e=trim(e);while(e.ends_with('/'))e.pop_back();auto u=parseUrl(e);
    if(u.path.find('?')!=std::string::npos)throw Error(ErrorCode::Configuration,"Endpoint must not contain query parameters.");
    if(!u.loopback&&u.scheme!="https")throw Error(ErrorCode::Configuration,"External models require HTTPS.");
    if(!u.loopback&&!remoteConsent)throw Error(ErrorCode::Consent,"Enable remote model consent in Settings before sending chat data.");
    return e;
}
bool safeSourceUrl(std::string const& s){try{auto u=parseUrl(s);return u.scheme=="https"&&!u.loopback;}catch(...){return false;}}
Json parseJson(std::string const& s){if(s.size()>2*1024*1024)throw Error(ErrorCode::Limit,"Response exceeds 2 MiB.");try{return Json::parse(s);}catch(Json::exception const&){throw Error(ErrorCode::InvalidData,"The server returned invalid JSON.");}}
std::string requiredString(Json const& j,char const* k,size_t max){if(!j.contains(k)||!j[k].is_string())throw Error(ErrorCode::InvalidData,std::string("Missing string: ")+k);auto s=j[k].get<std::string>();if(s.empty()||s.size()>max)throw Error(ErrorCode::InvalidData,std::string("Invalid length: ")+k);return s;}
int boundedInt(Json const& j,char const* k,int fallback,int lo,int hi){if(!j.contains(k))return fallback;if(!j[k].is_number_integer())throw Error(ErrorCode::InvalidData,std::string("Expected integer: ")+k);auto v=j[k].get<int64_t>();if(v<lo||v>hi)throw Error(ErrorCode::InvalidData,std::string("Out of range: ")+k);return static_cast<int>(v);}
void validateConfig(Config const& c){
    validateEndpoint(c.endpoint,c.allowRemote);
    if(c.model.empty()||c.model.size()>160)throw Error(ErrorCode::Configuration,"Choose a model in Settings / Wybierz model w ustawieniach.");
    if(c.timeoutSeconds<5||c.timeoutSeconds>300||c.maxTokens<128||c.maxTokens>4096||c.cacheSeconds<0||c.cacheSeconds>3600)
        throw Error(ErrorCode::Configuration,"Invalid model or cache limits.");
    // Publication provenance is documented separately. Local use is not Geode approval.
}
}
