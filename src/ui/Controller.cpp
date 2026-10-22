#include <gi/Controller.hpp>
#include <gi/Validation.hpp>
#include <gi/Text.hpp>
#include <Geode/loader/Mod.hpp>

namespace gi {
Controller::Controller(std::filesystem::path save,std::filesystem::path knowledge):saveDir(std::move(save)),knowledgeFile(std::move(knowledge)){}
View Controller::snapshot(bool includeMessages) const{
    std::lock_guard lock(state->mutex);auto const& v=state->view;
    View out;out.active=v.active;out.busy=v.busy;out.status=v.status;out.error=v.error;out.revision=v.revision;
    if(!includeMessages)return out;
    out.partial=v.partial;
    for(size_t n=0;n<v.chats.size();++n){
        auto const& chat=v.chats[n];Conversation copy{chat.id,chat.title,{}};
        if(n==v.active)for(auto const& m:chat.messages){
            Message item{m.role,m.content,m.time,{}};
            // UI needs attribution, not potentially large API JSON payloads.
            for(auto const& s:m.sources)item.sources.push_back({s.title,s.url,s.fetchedAt,s.confidence,s.cached,nullptr});
            copy.messages.push_back(std::move(item));
        }
        out.chats.push_back(std::move(copy));
    }
    return out;
}
bool Controller::launch(Work work){
    View initial;std::stop_token token;
    {std::lock_guard lock(state->mutex);if(state->view.busy)return false;
        state->stop=std::stop_source();token=state->stop.get_token();
        state->view.busy=true;state->view.error.clear();state->view.partial.clear();
        state->view.status="Working... / Pracuję...";++state->view.revision;initial=state->view;
    }
    auto shared=state;
    try { std::thread([shared,initial=std::move(initial),token,work=std::move(work)]()mutable{
        auto progress=[shared](std::string s){std::lock_guard lock(shared->mutex);shared->view.status=std::move(s);++shared->view.revision;};
        auto stream=[shared](std::string const& s){std::lock_guard lock(shared->mutex);shared->view.partial=s;++shared->view.revision;};
        try{work(initial,token,progress,stream);checkCancelled(token);initial.status="Ready / Gotowe";}
        catch(Error const& e){initial.error=token.stop_requested()?"Stopped / Zatrzymano":e.what();initial.status="Check message / Sprawdź komunikat";}
        catch(std::exception const&){initial.error="Operation failed. Invalid provider data or local storage error. / Błąd danych lub zapisu.";initial.status="Error / Błąd";}
        catch(...){initial.error="Unexpected error / Nieoczekiwany błąd";initial.status="Error / Błąd";}
        std::lock_guard lock(shared->mutex);
        initial.busy=false;initial.partial.clear();initial.revision=shared->view.revision+1;
        shared->view=std::move(initial);
    }).detach(); }
    catch(std::exception const&){std::lock_guard lock(state->mutex);state->view.busy=false;state->view.error="Cannot start background worker / Nie można uruchomić zadania";++state->view.revision;return false;}
    return true;
}
void Controller::stop(){std::lock_guard lock(state->mutex);if(!state->view.busy)return;state->stop.request_stop();state->view.status="Stopping... / Zatrzymywanie...";++state->view.revision;}
void Controller::persist(View const& v,std::filesystem::path const& path,bool enabled){HistoryStore store(path);if(enabled)store.save(v.chats);else store.erase();}
void Controller::load(bool enabled){auto dir=saveDir;launch([dir,enabled](View& v,auto,auto const&,auto const&){
    HistoryStore store(dir);if(enabled)v.chats=store.load();else store.erase();
    if(v.chats.empty())v.chats.push_back({newId(),"New chat / Nowa rozmowa",{}});v.active=0;
});}
void Controller::ask(std::string question,Json context,Config config){
    question=trim(question);if(question.empty()||question.size()>6000)return;
    {std::lock_guard lock(state->mutex);if(state->view.busy)return;
        if(state->view.chats.empty())state->view.chats.push_back({newId(),"New chat",{}});
        auto& chat=state->view.chats[state->view.active];
        if(chat.messages.size()>=78){state->view.error="This chat is full (80 messages). Start a new chat.";++state->view.revision;return;}
        if(chat.messages.empty())chat.title=clip(question,80);
        chat.messages.push_back({"user",question,utcNow(),{}});++state->view.revision;
    }
    auto dir=saveDir,file=knowledgeFile;
    launch([config,context=std::move(context),dir,file](View& v,auto stop,auto const& progress,auto const& stream){
        persist(v,dir,config.saveHistory);
        GeodeHttp transport;Http http(transport);Cache cache(dir,config.cacheEnabled);Knowledge knowledge(file);
        auto key=Credentials::read(Credentials::modelTarget(config.endpoint));
        auto searchKey=Credentials::read(Credentials::searchTarget);
        Api api(http,cache,knowledge,config,std::move(searchKey));Engine engine(http,api,config,std::move(key));
        auto result=engine.ask(v.chats[v.active].messages,context,stop,progress,stream);checkCancelled(stop);
        v.chats[v.active].messages.push_back({"assistant",std::move(result.text),utcNow(),std::move(result.sources)});
        persist(v,dir,config.saveHistory);
    });
}
void Controller::connection(Config config){launch([config](View& v,auto stop,auto const& progress,auto const&){
    progress("Checking model server... / Sprawdzam serwer...");
    GeodeHttp transport;Http http(transport);Request req;
    req.url=validateEndpoint(config.endpoint,config.allowRemote)+"/models";req.timeout=10;
    auto key=Credentials::read(Credentials::modelTarget(config.endpoint));if(!key.empty())req.headers["Authorization"]="Bearer "+key;
    auto j=http.json(req,stop);if(!j.is_object()||!j.contains("data")||!j["data"].is_array())throw Error(ErrorCode::Model,"Invalid models list.");
    std::string found;bool selected=false;
    for(auto const& m:j["data"]){auto id=requiredString(m,"id",160);found+=id+"\n";if(id==config.model)selected=true;if(found.size()>2000)break;}
    v.error=(selected?"Connected. Selected model found. / Połączono.\n":"Connected, but selected model is missing. / Brak wybranego modelu.\n")+found;
});}
void Controller::newChat(bool enabled){auto dir=saveDir;launch([dir,enabled](View& v,auto,auto const&,auto const&){
    if(v.chats.size()>=20)throw Error(ErrorCode::Limit,"20 chats maximum. Delete an old chat first.");
    v.chats.push_back({newId(),"New chat / Nowa rozmowa",{}});v.active=v.chats.size()-1;persist(v,dir,enabled);
});}
void Controller::select(size_t index){std::lock_guard lock(state->mutex);if(!state->view.busy&&index<state->view.chats.size()){state->view.active=index;++state->view.revision;}}
void Controller::removeChat(bool enabled){auto dir=saveDir;launch([dir,enabled](View& v,auto,auto const&,auto const&){
    if(!v.chats.empty())v.chats.erase(v.chats.begin()+v.active);
    if(v.chats.empty())v.chats.push_back({newId(),"New chat / Nowa rozmowa",{}});v.active=0;persist(v,dir,enabled);
});}
void Controller::clear(bool includeCache){auto dir=saveDir;launch([dir,includeCache](View& v,auto,auto const&,auto const&){
    HistoryStore(dir).erase();if(includeCache){Cache cache(dir,true);cache.clear();}
    v.chats={{newId(),"New chat / Nowa rozmowa",{}}};v.active=0;
});}
void Controller::secret(std::string target,std::string value){launch([target=std::move(target),value=std::move(value)](View&,auto,auto const&,auto const&){
    if(value.empty())Credentials::erase(target);else Credentials::write(target,value);
});}
Config readConfig(){
    auto* m=geode::Mod::get();Config c;
    c.endpoint=m->getSettingValue<std::string>("endpoint");c.model=m->getSettingValue<std::string>("model");c.language=m->getSettingValue<std::string>("language");
    c.allowRemote=m->getSettingValue<bool>("remote-consent");c.allowPublicApis=m->getSettingValue<bool>("public-consent");c.allowContext=m->getSettingValue<bool>("game-context");
    c.saveHistory=m->getSettingValue<bool>("history");c.cacheEnabled=m->getSettingValue<bool>("cache");c.streaming=m->getSettingValue<bool>("streaming");
    c.pointercrateEnabled=m->getSettingValue<bool>("pointercrate");c.geodeEnabled=m->getSettingValue<bool>("geode-index");c.musicEnabled=m->getSettingValue<bool>("music-search");
    c.webEnabled=m->getSettingValue<bool>("web-search");c.levelsEnabled=m->getSettingValue<bool>("level-search");
    c.timeoutSeconds=static_cast<int>(m->getSettingValue<int64_t>("timeout"));c.maxTokens=static_cast<int>(m->getSettingValue<int64_t>("response-tokens"));c.cacheSeconds=static_cast<int>(m->getSettingValue<int64_t>("cache-seconds"));
    return c;
}
Controller& controller(){static auto* c=new Controller(geode::Mod::get()->getSaveDir(),geode::Mod::get()->getResourcesDir()/"knowledge.json");return *c;}
}
