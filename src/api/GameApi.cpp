#include <gi/Api.hpp>
#include <gi/Validation.hpp>
#include <gi/Text.hpp>
namespace gi {
namespace {
std::map<std::string,std::string> pairs(std::string const& raw,std::string const& delimiter){auto p=split(raw,delimiter);if(p.size()%2)throw Error(ErrorCode::InvalidData,"GD returned an incomplete key/value record.");std::map<std::string,std::string> out;for(size_t i=0;i<p.size();i+=2)out[p[i]]=p[i+1];return out;}
int64_t field(std::map<std::string,std::string> const& p,char const* key){auto it=p.find(key);return it==p.end()?0:number(it->second);}
std::string str(std::map<std::string,std::string> const& p,char const* key){auto it=p.find(key);return it==p.end()?"":clip(plainText(it->second),4000);}
}
Json parseLevels(std::string const& raw){
    if(trim(raw)=="-1")return {{"levels",Json::array()},{"note","No levels returned by GD; this does not establish deletion or non-existence."}};
    auto sections=split(raw,"#");if(sections.size()<2||raw.starts_with('<'))throw Error(ErrorCode::InvalidData,"Unexpected GD level response (not level data).");
    std::map<std::string,std::string> authors;
    for(auto const& a:split(sections[1],"|")){auto p=split(a,":");if(p.size()>=2)authors[p[0]]=plainText(p[1]);}
    Json result=Json::array();
    for(auto const& item:split(sections[0],"|")){
        if(item.empty())continue;auto p=pairs(item,":");auto id=field(p,"1");auto name=str(p,"2");if(id<=0||name.empty())throw Error(ErrorCode::InvalidData,"GD level is missing ID or name.");
        auto length=field(p,"15");bool platformer=length==5;auto demon=field(p,"17")!=0;
        std::string difficulty="N/A",demonDifficulty;
        if(field(p,"25"))difficulty="Auto";
        else if(demon){difficulty="Demon";switch(field(p,"43")){case 3:demonDifficulty="Easy";break;case 4:demonDifficulty="Medium";break;case 5:demonDifficulty="Insane";break;case 6:demonDifficulty="Extreme";break;default:demonDifficulty="Hard";}}
        else {auto denominator=field(p,"8");auto value=denominator>0?field(p,"9")/denominator:0;static const std::vector<std::string> d={"N/A","Easy","Normal","Hard","Harder","Insane"};if(value>=0&&value<static_cast<int64_t>(d.size()))difficulty=d[static_cast<size_t>(value)];}
        static const std::vector<std::string> lengths={"Tiny","Short","Medium","Long","XL","Platformer"};
        auto epic=field(p,"42");std::string rating=epic==3?"Mythic":epic==2?"Legendary":epic==1?"Epic":field(p,"19")>0?"Featured":"None";
        std::string description;try{description=base64Decode(str(p,"3"));}catch(Error const&){description="[Invalid description encoding]";}
        Json row={{"id",id},{"name",name},{"creator_id",field(p,"6")},{"creator",authors.contains(str(p,"6"))?authors[str(p,"6")]:"Unknown"},
            {"description",description},{"difficulty",difficulty},{"demon_difficulty",demonDifficulty},{"mode",platformer?"platformer":"classic"},
            {"stars",platformer?0:field(p,"18")},{"moons",platformer?field(p,"18"):0},{"length",length>=0&&length<6?lengths[static_cast<size_t>(length)]:"Unknown"},
            {"song_id",field(p,"35")},{"official_song_index",field(p,"12")},{"rating",rating},{"downloads",field(p,"10")},{"likes_net",field(p,"14")},{"version",field(p,"5")}};
        result.push_back(std::move(row));if(result.size()>=10)break;
    }
    return {{"levels",result},{"note","Game server protocol is community documented, not an official supported public REST API. Missing data is not proof of absence."}};
}
Json parseSong(std::string const& raw,int id){
    auto text=trim(raw);if(text=="-1"||text=="-2")return {{"song_id",id},{"gd_available","unverified"},{"server_code",text},{"note","The server did not provide usable song metadata; no licence or whitelist conclusion."}};
    auto p=pairs(text,"~|~");if(field(p,"1")!=id||str(p,"2").empty()||str(p,"4").empty())throw Error(ErrorCode::InvalidData,"GD returned an unexpected song ID or invalid metadata.");
    bool hasDownload=str(p,"10").starts_with("https%3A")||str(p,"10").starts_with("https://")||str(p,"10").starts_with("http%3A");
    return {{"song_id",id},{"title",decode(str(p,"2"))},{"author",decode(str(p,"4"))},{"url","https://www.newgrounds.com/audio/listen/"+std::to_string(id)},
        {"gd_available",hasDownload?"metadata_and_download_link_returned":"unverified"},{"licence","not_verified"},{"genre",nullptr},{"mood",nullptr},
        {"note","A returned download link is availability evidence at fetch time; no download or playback test is performed. This is not a licence for use outside GD."}};
}
Source Api::levels(Json const& args,std::stop_token stop){
    auto query=requiredString(args,"query",100);auto page=boundedInt(args,"page",0,0,20);
    const std::string url="https://www.boomlings.com/database/getGJLevels21.php";
    auto key="levels:"+query+":"+std::to_string(page);if(auto c=cache.get(key))return *c;
    Request r;r.method="POST";r.url=url;r.headers={{"Content-Type","application/x-www-form-urlencoded"}};
    // This published client protocol identifier is not a user/API credential.
    r.body="secret=Wmfd2893gb7&gameVersion=22&binaryVersion=45&gdw=0&type=0&str="+encode(query)+"&page="+std::to_string(page)+"&len=-&diff=-";
    Source s{"Geometry Dash level search",url,utcNow(),"game-server",false,parseLevels(http.request(r,stop).body)};
    cache.put(key,s,std::min(300,config.cacheSeconds));return s;
}
Source Api::song(Json const& args,std::stop_token stop){
    auto id=boundedInt(args,"id",0,1,2000000000);if(id==0)throw Error(ErrorCode::InvalidData,"Song ID is required.");
    auto key="song:"+std::to_string(id);if(auto c=cache.get(key))return *c;
    Request r;r.method="POST";r.url="https://www.boomlings.com/database/getGJSongInfo.php";r.headers={{"Content-Type","application/x-www-form-urlencoded"}};r.body="secret=Wmfd2893gb7&songID="+std::to_string(id);
    Source s{"GD song metadata",r.url,utcNow(),"game-server",false,parseSong(http.request(r,stop).body,id)};cache.put(key,s,std::min(600,config.cacheSeconds));return s;
}
}
