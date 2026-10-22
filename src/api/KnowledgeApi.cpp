#include <gi/Api.hpp>
#include <gi/Text.hpp>
#include <gi/Validation.hpp>
#include <regex>
namespace gi {
namespace {
Json objectPayload(Json const& j){auto const& x=j.contains("data")?j.at("data"):j;if(!x.is_object())throw Error(ErrorCode::InvalidData,"API returned an invalid object.");return x;}
Json demons(Json const& j){if(!j.is_array()||j.size()>100)throw Error(ErrorCode::InvalidData,"Pointercrate returned an invalid list.");for(auto const& x:j){requiredString(x,"name");if(!x.contains("id")||!x["id"].is_number_integer()||!x.contains("position")||(!x["position"].is_null()&&!x["position"].is_number_integer()))throw Error(ErrorCode::InvalidData,"Invalid Pointercrate demon fields.");}return j;}
Json array(Json const& j){if(!j.is_array()||j.size()>100)throw Error(ErrorCode::InvalidData,"Expected API array.");return j;}
std::string argument(Json const& j,char const* key,std::string fallback){if(!j.contains(key))return fallback;return requiredString(j,key);}
Json tool(char const* name,char const* description,Json properties,Json required=Json::array()){
    return {{"type","function"},{"function",{{"name",name},{"description",description},{"parameters",{{"type","object"},{"properties",properties},{"required",required},{"additionalProperties",false}}}}}};
}
}
Source Api::fetch(std::string const& title,std::string const& url,int ttl,std::stop_token stop,std::function<Json(Json const&)> validate){
    if(auto cached=cache.get(url))return *cached;
    Request r;r.url=url;r.headers["Accept"]="application/json";
    auto data=validate(http.json(r,stop));Source s{title,url,utcNow(),"primary",false,std::move(data)};cache.put(url,s,std::min(ttl,config.cacheSeconds));return s;
}
Source Api::demonlist(Json const& a,std::stop_token stop){
    auto op=argument(a,"operation","top");const std::string root="https://pointercrate.com/api/";
    if(op=="history")return {"Local Pointercrate observations","https://pointercrate.com/demonlist/",utcNow(),"local-observations",false,cache.history("top10")};
    if(op=="top"){
        auto count=boundedInt(a,"count",10,1,100),position=boundedInt(a,"position",1,1,10000);
        auto url=root+"v2/demons/listed/?limit="+std::to_string(count)+"&after="+std::to_string(position-1);
        auto s=fetch("Pointercrate ranking",url,120,stop,demons);
        if(position==1&&count==10)cache.observe("top10",s);
        return s;
    }
    if(op=="search")return fetch("Pointercrate demon search",root+"v2/demons/?name_contains="+encode(requiredString(a,"name",100))+"&limit=20",120,stop,demons);
    if(op=="detail"){
        auto id=boundedInt(a,"id",0,1,2000000000);if(!id)throw Error(ErrorCode::InvalidData,"Pointercrate demon ID required (not GD level ID).");
        return fetch("Pointercrate demon, creators and records",root+"v2/demons/"+std::to_string(id)+"/",120,stop,[](Json const& j){auto x=objectPayload(j);requiredString(x,"name");if(!x.contains("creators")||!x["creators"].is_array())throw Error(ErrorCode::InvalidData,"Missing creators list.");return x;});
    }
    if(op=="player"){
        if(a.contains("id")){auto id=boundedInt(a,"id",0,1,2000000000);return fetch("Pointercrate player",root+"v1/players/"+std::to_string(id)+"/",300,stop,objectPayload);}
        return fetch("Pointercrate players",root+"v1/players/?name_contains="+encode(requiredString(a,"name",100))+"&limit=20",300,stop,array);
    }
    if(op=="records"){
        auto id=boundedInt(a,"id",0,1,2000000000);if(!id)throw Error(ErrorCode::InvalidData,"Pointercrate demon ID is required.");
        // Anonymous requests expose public records. Explicit status filtering needs moderator permission.
        return fetch("Pointercrate public records (first page)",root+"v1/records/?demon_id="+std::to_string(id)+"&limit=20",120,stop,array);
    }
    throw Error(ErrorCode::InvalidData,"Unknown Demonlist operation.");
}
Source Api::mods(Json const& a,std::stop_token stop){
    const std::string root="https://api.geode-sdk.org/v1/mods";
    std::string url=root;
    if(a.contains("id")){
        auto id=requiredString(a,"id",100);if(!std::regex_match(id,std::regex("[a-z0-9_.-]+")))throw Error(ErrorCode::InvalidData,"Invalid Geode mod ID.");url+="/"+id;
    } else {
        auto query=a.contains("query")?requiredString(a,"query",100):"";
        auto tag=a.contains("tag")?requiredString(a,"tag",40):"";
        if(!tag.empty()&&!std::regex_match(tag,std::regex("[a-z-]+")))throw Error(ErrorCode::InvalidData,"Invalid mod tag.");
        url+="?query="+encode(query)+(tag.empty()?"":"&tags="+encode(tag))+"&gd="+encode(config.gdVersion)+"&platforms=win&geode="+encode(config.geodeVersion)+"&per_page=10&page="+std::to_string(boundedInt(a,"page",1,1,50));
    }
    auto result=fetch("Official Geode mod index",url,600,stop,[](Json const& j){
        if(!j.is_object()||!j.contains("payload")||(!j["payload"].is_object()))throw Error(ErrorCode::InvalidData,"Invalid Geode index payload.");
        if(j.contains("error")&&j["error"].is_string()&&!j["error"].get<std::string>().empty())throw Error(ErrorCode::InvalidData,"Geode API reported an error.");
        Json items=j["payload"].contains("data")?j["payload"]["data"]:Json::array({j["payload"]});if(!items.is_array())throw Error(ErrorCode::InvalidData,"Invalid mod result list.");
        Json out=Json::array();for(auto const& m:items){auto id=requiredString(m,"id",100);if(!m.contains("versions")||!m["versions"].is_array())throw Error(ErrorCode::InvalidData,"Missing mod versions.");Json versions=Json::array();for(auto const& v:m["versions"]){Json x;for(auto const* key:{"name","version","description","geode","gd","status"})if(v.contains(key))x[key]=v[key];versions.push_back(x);if(versions.size()>=3)break;}
            out.push_back({{"id",id},{"url","https://geode-sdk.org/mods/"+id},{"versions",versions},{"developers",m.value("developers",Json::array())},{"tags",m.value("tags",Json::array())},{"about",m.contains("about")&&m["about"].is_string()?clip(m["about"].get<std::string>(),4500):""}});}
        return Json{{"mods",out},{"count",j["payload"].value("count",static_cast<int>(out.size()))},{"compatibility_note","Search is server-filtered for this GD/Geode/Windows version. This is declared compatibility, not proof that it works with every installed mod. Detail results require checking gd.win and geode constraints."}};
    });return result;
}
Source Api::search(Json const& a,std::stop_token stop){
    auto query=requiredString(a,"query",300);auto scope=argument(a,"scope","geometry_dash");
    if(scope!="music"&&scope!="geometry_dash")throw Error(ErrorCode::InvalidData,"Unknown search scope.");
    if(searchKey.empty())return freeSearch(query,scope,stop);
    auto full=query+(scope=="music"?" site:newgrounds.com/audio/listen/":" Geometry Dash");
    auto url="https://api.search.brave.com/res/v1/web/search?q="+encode(full)+"&count=6&safesearch=strict&text_decorations=false";
    // Search-provider terms vary by plan: results are not persisted in the public API cache.
    Request r;r.url=url;r.headers={{"Accept","application/json"},{"X-Subscription-Token",searchKey}};
    auto data=http.json(r,stop);if(!data.is_object())throw Error(ErrorCode::InvalidData,"Invalid search response.");
    Json out=Json::array();if(data.contains("web")){if(!data["web"].is_object()||!data["web"].contains("results")||!data["web"]["results"].is_array())throw Error(ErrorCode::InvalidData,"Invalid search result list.");
        for(auto const& x:data["web"]["results"]){auto link=requiredString(x,"url",2048);if(!safeSourceUrl(link))continue;auto u=parseUrl(link);
            Json row={{"title",clip(plainText(requiredString(x,"title",2000)),300)},{"url",link},{"snippet",x.contains("description")&&x["description"].is_string()?clip(plainText(x["description"]),1000):""},{"verification","search_snippet_only"}};
            if(scope=="music"){std::smatch m;if(u.host!="www.newgrounds.com"&&u.host!="newgrounds.com")continue;if(!std::regex_match(u.path,m,std::regex(R"(/audio/listen/([0-9]+)(?:[/?].*)?)")))continue;row["song_id"]=number(m[1]);row["author"]=nullptr;row["genre"]=nullptr;row["mood"]=nullptr;row["licence"]="not_verified";row["gd_available"]="unverified";}
            out.push_back(row);if(out.size()>=6)break;
        }
    }
    return {"Brave Search: "+query,"https://search.brave.com/search?q="+encode(full),utcNow(),"search-snippets",false,{{"results",out},{"note","Search snippets are discovery evidence. For music call song_info for actual title, author and GD metadata. Do not invent mood, licence or audio similarity; no audio analysis is performed."}}};
}
std::vector<Source> Api::call(std::string const& name,Json const& a,std::stop_token stop){
    checkCancelled(stop);if(!a.is_object()||a.dump().size()>4096)throw Error(ErrorCode::InvalidData,"Tool arguments must be a small JSON object.");
    if(name=="editor_knowledge"){
        auto language=argument(a,"language",config.language);
        if(language!="pl"&&language!="en")throw Error(ErrorCode::InvalidData,"Knowledge language must be pl or en.");
        return knowledge.search(requiredString(a,"query",300),4,language);
    }
    if(!config.allowPublicApis)throw Error(ErrorCode::Consent,"Enable public API consent in Settings to search external sources. Only query terms and IDs are sent to these services.");
    if(name=="demonlist"&&config.pointercrateEnabled)return {demonlist(a,stop)};
    if(name=="geode_mods"&&config.geodeEnabled)return {mods(a,stop)};
    if(name=="levels"&&config.levelsEnabled)return {levels(a,stop)};
    if(name=="song_info"&&config.musicEnabled)return {song(a,stop)};
    if(name=="web_search"&&config.webEnabled){
        if(a.value("scope","")=="music"&&!config.musicEnabled)throw Error(ErrorCode::Consent,"Music search is disabled in Settings.");
        return {search(a,stop)};
    }
    throw Error(ErrorCode::InvalidData,"Tool is not on the read-only allowlist.");
}
Json Api::tools(){
    auto s=Json{{"type","string"}},i=Json{{"type","integer"}};
    return Json::array({
        tool("demonlist","Read live Pointercrate. top supports count and starting position. search uses name. detail/records require Pointercrate demon ID (not level_id). detail lists creators; publisher is NOT the creators list. player takes name or player id. history contains only two local top10 observations, if available.",{{"operation",{{"type","string"},{"enum",{"top","search","detail","player","records","history"}}}},{"name",s},{"id",i},{"count",i},{"position",i}},{"operation"}),
        tool("geode_mods","Search the official Geode index using a short name query or a functional tag (editor, music, utility, performance). Use query in English; compare versions and authors. id requests details for a known mod ID. page starts at 1.",{{"query",s},{"tag",s},{"id",s},{"page",i}}),
        tool("levels","Read public GD level metadata by name or numeric ID string; pages start at 0. Use the current game-context level ID if user asks about this level. Contains difficulty, stars/moons, song ID and available metadata only.",{{"query",s},{"page",i}},{"query"}),
        tool("song_info","Verify a Newgrounds numeric Song ID against GD song metadata. Returns title and author if accessible. Does not establish a licence. Never guess the ID.",{{"id",i}},{"id"}),
        tool("web_search","Free catalogue search: music searches MusicBrainz metadata (no GD Song IDs or availability), geometry_dash searches Wikipedia. Optional Brave key enables broader web search and Newgrounds discovery. Use short search terms, not a whole question. Mood matching is text inference, never audio analysis.",{{"query",s},{"scope",{{"type","string"},{"enum",{"music","geometry_dash"}}}}},{"query","scope"}),
        tool("game_context","Read the user-consented snapshot of the level visible when the assistant was opened. Returns only available level metadata; no network and no objects. Unavailable when consent is off or no level is open.",Json::object()),
        tool("editor_knowledge","Retrieve local referenced GD 2.2 notes and Geode troubleshooting facts. Use for triggers, groups, cameras, mechanics and historical facts. Specify language pl or en matching the user's question.",{{"query",s},{"language",{{"type","string"},{"enum",{"pl","en"}}}}},{"query"})});
}
}
