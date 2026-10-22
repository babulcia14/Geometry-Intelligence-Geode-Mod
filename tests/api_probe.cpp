#include <gi/Engine.hpp>
#include <gi/Text.hpp>
#include <iostream>
using namespace gi;
int main(int argc,char** argv){
    try{
        auto root=std::filesystem::current_path();WinHttp transport;Http http(transport);
        Config config;config.endpoint="http://127.0.0.1:18765/v1";config.model="gi-tiny";config.allowPublicApis=true;config.streaming=true;config.timeoutSeconds=60;
        Cache cache(root/"test-output/probe",false);Knowledge knowledge(root/"resources/knowledge.json");Api api(http,cache,knowledge,config);
        if(argc>1&&std::string(argv[1])=="chat"){
            Engine engine(http,api,config,"");std::vector<Message> history;
            for(auto const* q:{"What is the current Top 1 Demon?","Who created it?","How do I make an object rotate forever?"}){
                history.push_back({"user",q,utcNow(),{}});size_t chunks=0;
                auto a=engine.ask(history,{},{},[](auto){},[&](auto const&){++chunks;});
                std::cout<<Json{{"question",q},{"text",a.text},{"sources",a.sources},{"stream_updates",chunks}}.dump(-1,' ',true)<<'\n';
                history.push_back({"assistant",a.text,utcNow(),a.sources});
            }
        }else{
            std::vector<std::pair<std::string,Json>> cases={
                {"demonlist",{{"operation","top"},{"count",10}}},
                {"demonlist",{{"operation","search"},{"name","Bloodbath"}}},
                {"demonlist",{{"operation","player"},{"name","Zoink"}}},
                {"geode_mods",{{"query","BetterEdit"}}},
                {"levels",{{"query","Nine Circles"}}},
                {"song_info",{{"id",476613}}},
                {"web_search",{{"query","ambient"},{"scope","music"}}},
                {"web_search",{{"query","history"},{"scope","geometry_dash"}}}
            };
            for(auto const& [name,args]:cases){try{auto s=api.call(name,args,{});std::cout<<Json{{"tool",name},{"args",args},{"status","PASS"},{"sources",s}}.dump(-1,' ',true)<<'\n';}catch(std::exception const& e){std::cout<<Json{{"tool",name},{"args",args},{"status","FAIL"},{"error",e.what()}}.dump(-1,' ',true)<<'\n';}}
        }
    }catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
