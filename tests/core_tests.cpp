#include <gi/Engine.hpp>
#include <gi/Stream.hpp>
#include <gi/Validation.hpp>
#include <gi/Text.hpp>
#include <iostream>
#include <deque>
#include <fstream>

using namespace gi;
int passed=0,failed=0;
void expect(bool condition,std::string const& name){if(condition){++passed;std::cout<<"PASS "<<name<<'\n';}else{++failed;std::cout<<"FAIL "<<name<<'\n';}}
template<class F>void rejects(F f,std::string const& name){bool rejected=false;try{f();}catch(...){rejected=true;}expect(rejected,name);}
struct Fake final:Transport{
    std::deque<Response> responses;std::vector<Request> requests;
    Response perform(Request const& r,std::stop_token stop)override{
        checkCancelled(stop);requests.push_back(r);
        if(responses.empty())throw Error(ErrorCode::Network,"Test transport has no response");
        auto out=responses.front();responses.pop_front();
        if(r.onChunk&&out.status==200)for(size_t n=0;n<out.body.size();n+=3)r.onChunk(std::string_view(out.body).substr(n,3));
        return out;
    }
};
std::string event(Json delta,Json finish=nullptr){return "data: "+Json{{"choices",Json::array({{{"delta",delta},{"finish_reason",finish}}})}}.dump()+"\r\n\r\n";}
int main(int argc,char** argv){
    if(argc<2)return 2;std::filesystem::path root=argv[1],dir=root/"test-output"/newId();
    std::filesystem::create_directories(dir);
    expect(encode("a &ą")=="a%20%26%C4%85","UTF-8 URL encoding");
    expect(decode("a%20b%2B") == "a b+","URL decoding");
    expect(clip("ąą",3)=="ą","UTF-8 truncation boundary");
    expect(number("12bad",7)==7,"Reject trailing integer garbage");
    expect(base64Decode("SGVsbG8=")=="Hello","Description base64");
    rejects([]{base64Decode("!!!!");},"Malformed base64 rejected");
    expect(parseUrl("http://127.0.0.1:18765/v1").loopback,"Loopback URL recognized");
    expect(!parseUrl("https://localhost.evil.example/v1").loopback,"Loopback lookalike not trusted");
    rejects([]{validateEndpoint("https://example.com/v1",false);},"Remote chat requires consent");
    rejects([]{validateEndpoint("http://example.com/v1",true);},"Remote cleartext rejected");
    rejects([]{validateEndpoint("https://key@example.com/v1",true);},"URL credentials rejected");
    rejects([]{validateEndpoint("http://127.0.0.1:18765/v1?key=secret",false);},"Endpoint query rejected");
    expect(!safeSourceUrl("javascript:alert(1)"),"Unsafe source scheme rejected");
    rejects([]{parseJson("<html>blocked</html>");},"Invalid API JSON rejected");
    rejects([]{boundedInt(Json{{"n",1000}},"n",0,1,100);},"Tool bounds enforced");
    std::stop_source stop;stop.request_stop();rejects([&]{checkCancelled(stop.get_token());},"Cancellation token honored");

    Fake fake;Http http(fake);Request r;r.url="https://example.com/data";
    fake.responses.push_back({429,"",{{"retry-after","20"}}});rejects([&]{http.request(r,{});},"HTTP 429 surfaced");
    rejects([&]{http.request(r,{});},"Rate-limit backoff prevents next request");expect(fake.requests.size()==1,"No transport call during backoff");
    r.url="https://another.example/data";fake.responses.push_back({200,"abcdef",{}});r.maxBytes=3;rejects([&]{http.request(r,{});},"Body limit enforced");
    r.maxBytes=1024;fake.responses.push_back({500,"error",{}});rejects([&]{http.request(r,{});},"HTTP 500 surfaced");
    Cache cache(dir,true);Source source{"test","https://example.com/data",utcNow(),"primary",false,{{"value",42}}};
    cache.put("x",source,60);expect(cache.get("x")->cached,"Cache hit labeled");expect(cache.get("x")->fetchedAt==source.fetchedAt,"Cache preserves retrieval date");
    cache.put("expired",source,0);expect(!cache.get("expired"),"Expired data not used as current");
    Cache reloaded(dir,true);expect(reloaded.get("x")->data["value"]==42,"Cache persisted and reloaded");
    Cache off(dir,false);expect(!off.get("x"),"Cache opt-out honored");
    cache.clear();expect(!cache.get("x"),"Clear cache");
    HistoryStore history(dir);Conversation chat{"test","test",{{"user","prywatne",utcNow(),{}}}};
    history.save({chat});expect(history.load()[0].messages[0].content=="prywatne","Encrypted history round trip");
    expect(readBounded(dir/"history.dpapi").find("prywatne")==std::string::npos,"History has no plaintext message");
    atomicWrite(dir/"history.dpapi","damaged");rejects([&]{history.load();},"Corrupt history fails without replacement");
    expect(readBounded(dir/"history.dpapi")=="damaged","Corrupt original preserved");history.erase();expect(history.load().empty(),"History deletion");

    auto levels=parseLevels("1:123:2:Sample:3:SGVsbG8=:6:7:8:10:9:50:15:5:18:8:35:456:42:2#7:Author:99#");
    expect(levels["levels"][0]["moons"]==8&&levels["levels"][0]["stars"]==0,"Platformer moons parsing");
    expect(levels["levels"][0]["creator"]=="Author","Level creator mapping");
    expect(levels["levels"][0]["rating"]=="Legendary","Level rating tier parsing");
    expect(parseLevels("-1")["levels"].empty(),"Empty level search is not fabricated");
    rejects([]{parseLevels("<html>bad</html>");},"HTML is not accepted as level data");
    auto song=parseSong("1~|~123~|~2~|~Real%20Title~|~4~|~Artist~|~10~|~https%3A%2F%2Fexample.com%2Ffile",123);
    expect(song["title"]=="Real Title","Song title decoded");
    expect(song["genre"].is_null()&&song["mood"].is_null(),"Missing song fields not invented");
    expect(parseSong("-2",123)["gd_available"]=="unverified","Unavailable song is marked unverified");
    rejects([]{parseSong("1~|~321~|~2~|~Song~|~4~|~Author",123);},"Mismatched Song ID rejected");

    std::string received;ChatStream stream([&](auto const& text){received=text;});
    auto sse=event({{"role","assistant"}})+event({{"content","Cześć"}})+event({{"content"," świecie"}})+event(Json::object(),"stop")+"data: [DONE]\n\n";
    for(char c:sse)stream.feed(std::string_view(&c,1));
    expect(stream.response()["choices"][0]["message"]["content"]=="Cześć świecie","SSE fragmented UTF-8 preserved");
    expect(received=="Cześć świecie","SSE real-time text callback");
    ChatStream cut;cut.feed(event({{"content","partial"}}));rejects([&]{cut.response();},"Incomplete SSE is not success");
    ChatStream tools;
    tools.feed(event({{"tool_calls",Json::array({{{"index",0},{"id","a"},{"function",{{"name","demonlist"},{"arguments","{\"operation\":"}}}}})}}));
    tools.feed(event({{"tool_calls",Json::array({{{"index",0},{"function",{{"arguments","\"top\"}"}}}}})}}));
    tools.feed(event(Json::object(),"tool_calls")+"data: [DONE]\n\n");
    expect(tools.response()["choices"][0]["message"]["tool_calls"][0]["function"]["arguments"]=="{\"operation\":\"top\"}","SSE tool arguments assembled");

    Knowledge knowledge(root/"resources/knowledge.json");auto refs=knowledge.search("rotate forever loop");expect(!refs.empty(),"Editor references retrieved");
    Config config;config.endpoint="http://127.0.0.1:18765/v1";config.model="gi-tiny";config.streaming=false;
    auto context=Engine::contextMessages({{"user","Question",utcNow(),{}}},config,{{"secret","not enabled"}});
    expect(context.dump().find("not enabled")==std::string::npos,"Game context not sent without consent");
    config.allowContext=true;context=Engine::contextMessages({{"user","Question",utcNow(),{}}},config,{{"level_id",123}});
    expect(context.dump().find("123")!=std::string::npos,"Opted-in context included");
    Fake apiFake;Http apiHttp(apiFake);Cache testCache(dir,false);Api api(apiHttp,testCache,knowledge,config);
    rejects([&]{api.call("demonlist",{{"operation","top"}},{});},"No public network without permission");
    expect(api.call("editor_knowledge",{{"query","Move Trigger"}},{}).size()>0,"Offline knowledge needs no network consent");
    config.allowPublicApis=true;Api allowed(apiHttp,testCache,knowledge,config);
    rejects([&]{allowed.call("execute_code",Json::object(),{});},"Unknown tool denied");
    apiFake.responses.push_back({200,"[{\"id\":1,\"position\":1,\"name\":\"Fixture only\"}]",{}});
    auto live=allowed.call("demonlist",{{"operation","top"},{"count",1}},{});
    expect(live[0].data[0]["name"]=="Fixture only","Pointercrate is populated from HTTP transport");
    expect(apiFake.requests.back().url.find("/api/v2/demons/listed/?limit=1&after=0")!=std::string::npos,"Documented Pointercrate route");
    apiFake.responses.push_back({200,R"({"payload":{"data":[],"count":0}})",{}});
    allowed.call("geode_mods",{{"query","BetterEdit"}},{});
    expect(apiFake.requests.back().url.find("tags=")==std::string::npos,"Empty Geode tag omitted (live API rejects it)");
    apiFake.responses.push_back({200,R"({"recordings":[{"id":"00000000-0000-0000-0000-000000000001","title":"Fixture only","artist-credit":[{"name":"Test Artist"}]}]})",{}});
    auto music=allowed.call("web_search",{{"query","ambient"},{"scope","music"}},{});
    expect(music[0].data["results"][0]["song_id"].is_null()&&music[0].data["results"][0]["gd_available"]=="unverified","Music catalogue cannot invent Song IDs or GD availability");
    Fake model;Http modelHttp(model);Api modelApi(modelHttp,testCache,knowledge,config);Engine engine(modelHttp,modelApi,config,"");
    Json call={{"id","call1"},{"type","function"},{"function",{{"name","editor_knowledge"},{"arguments","{\"query\":\"Move Trigger\"}"}}}};
    model.responses.push_back({200,Json{{"choices",Json::array({{{"message",{{"role","assistant"},{"content",nullptr},{"tool_calls",Json::array({call})}}},{"finish_reason","tool_calls"}}})}}.dump(),{}});
    model.responses.push_back({200,Json{{"choices",Json::array({{{"message",{{"role","assistant"},{"content","Use target groups [1]."}}},{"finish_reason","stop"}}})}}.dump(),{}});
    auto answer=engine.ask({{"user","How does Move Trigger work?",utcNow(),{}}},{},{},[](auto){});
    expect(!answer.sources.empty()&&answer.text=="Use target groups [1].","Full model/tool/model orchestration");
    auto replay=parseJson(model.requests[1].body);expect(replay["messages"].back()["role"]=="tool","Tool evidence replayed to language engine");
    std::cout<<"RESULT "<<passed<<" passed, "<<failed<<" failed\n";return failed?1:0;
}
