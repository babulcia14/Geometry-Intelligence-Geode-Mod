#include <gi/Text.hpp>
#include <algorithm>
#include <charconv>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <random>
#include <sstream>
namespace gi {
std::string trim(std::string s) {
    auto space=[](unsigned char c){return std::isspace(c)!=0;};
    s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), space));
    s.erase(std::find_if_not(s.rbegin(), s.rend(), space).base(), s.end());
    return s;
}
std::string lower(std::string s) {
    std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    return s;
}
std::string encode(std::string_view s) {
    static constexpr char hex[]="0123456789ABCDEF";
    std::string out;
    for(unsigned char c:s) {
        if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c=='~') out+=c;
        else {out+='%'; out+=hex[c>>4]; out+=hex[c&15];}
    }
    return out;
}
std::string decode(std::string_view s) {
    auto hex=[](char c)->int{if(c>='0'&&c<='9')return c-'0'; if(c>='a'&&c<='f')return c-'a'+10; if(c>='A'&&c<='F')return c-'A'+10; return -1;};
    std::string out;
    for(size_t i=0;i<s.size();++i) {
        if(s[i]=='%'&&i+2<s.size()&&hex(s[i+1])>=0&&hex(s[i+2])>=0) {out+=static_cast<char>(hex(s[i+1])*16+hex(s[i+2]));i+=2;}
        else out+=s[i]=='+'?' ':s[i];
    }
    return out;
}
std::string base64Decode(std::string_view s) {
    constexpr std::string_view alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out; unsigned value=0; int bits=0;
    for(char c:s) {
        if(c=='=')break;
        if(c=='-')c='+'; if(c=='_')c='/';
        auto pos=alphabet.find(c);
        if(pos==std::string_view::npos) throw Error(ErrorCode::InvalidData,"Invalid base64 description.");
        value=(value<<6)|static_cast<unsigned>(pos); bits+=6;
        if(bits>=8){bits-=8;out+=static_cast<char>((value>>bits)&255);}
    }
    return plainText(out);
}
std::vector<std::string> split(std::string_view s,std::string_view d) {
    std::vector<std::string> out;
    if(d.empty())return {std::string(s)};
    size_t start=0;
    for(;;){auto end=s.find(d,start); if(end==s.npos){out.emplace_back(s.substr(start));break;} out.emplace_back(s.substr(start,end-start));start=end+d.size();}
    return out;
}
int64_t epochNow(){return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
std::string utcNow(){auto t=std::time(nullptr);std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm,&t);
#else
    gmtime_r(&t,&tm);
#endif
    std::ostringstream s;s<<std::put_time(&tm,"%Y-%m-%d %H:%M:%S UTC");return s.str();
}
std::string newId(){std::random_device r;std::ostringstream s;s<<epochNow()<<'-'<<std::hex<<r()<<r();return s.str();}
std::string clip(std::string s,size_t limit){if(s.size()<=limit)return s;size_t end=limit;while(end>0&&(static_cast<unsigned char>(s[end])&0xc0)==0x80)--end;s.resize(end);return s;}
std::string plainText(std::string s){s.erase(std::remove_if(s.begin(),s.end(),[](unsigned char c){return c<32&&c!='\n'&&c!='\t';}),s.end());return clip(std::move(s),50000);}
int64_t number(std::string const& s,int64_t fallback){int64_t v{};auto [p,e]=std::from_chars(s.data(),s.data()+s.size(),v);return e==std::errc{}&&p==s.data()+s.size()?v:fallback;}
void to_json(Json& j,Source const& s){j={{"title",s.title},{"url",s.url},{"fetched_at",s.fetchedAt},{"confidence",s.confidence},{"cached",s.cached},{"data",s.data}};}
void from_json(Json const& j,Source& s){s.title=j.at("title");s.url=j.at("url");s.fetchedAt=j.at("fetched_at");s.confidence=j.value("confidence","primary");s.cached=j.value("cached",false);s.data=j.value("data",Json{});}
void to_json(Json& j,Message const& m){j={{"role",m.role},{"content",m.content},{"time",m.time},{"sources",m.sources}};}
void from_json(Json const& j,Message& m){m.role=j.at("role");m.content=j.at("content");m.time=j.at("time");m.sources=j.value("sources",std::vector<Source>{});}
void to_json(Json& j,Conversation const& c){j={{"id",c.id},{"title",c.title},{"messages",c.messages}};}
void from_json(Json const& j,Conversation& c){c.id=j.at("id");c.title=j.at("title");c.messages=j.at("messages").get<std::vector<Message>>();}
}
