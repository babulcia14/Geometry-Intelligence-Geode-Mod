#include <gi/Store.hpp>
#include <gi/Text.hpp>
#include <gi/Validation.hpp>
#include <fstream>
#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#endif
namespace gi {
void atomicWrite(std::filesystem::path const& path,std::string const& bytes){
    try {
        std::filesystem::create_directories(path.parent_path());
        auto temp=path;temp+=".tmp";
        {std::ofstream f(temp,std::ios::binary|std::ios::trunc);f.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));f.flush();if(!f)throw Error(ErrorCode::Storage,"Cannot write local data.");}
#ifdef _WIN32
        if(!MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw Error(ErrorCode::Storage,"Cannot replace local data file.");
#else
        std::filesystem::rename(temp,path);
#endif
    }catch(std::filesystem::filesystem_error const&){throw Error(ErrorCode::Storage,"Local storage is not writable.");}
}
std::string readBounded(std::filesystem::path const& path,size_t limit){
    try {if(!std::filesystem::exists(path))return {};if(std::filesystem::file_size(path)>limit)throw Error(ErrorCode::Storage,"Local data file is too large.");
        std::ifstream f(path,std::ios::binary);if(!f)throw Error(ErrorCode::Storage,"Cannot read local data.");return {std::istreambuf_iterator<char>(f),{}};
    }catch(std::filesystem::filesystem_error const&){throw Error(ErrorCode::Storage,"Cannot access local storage.");}
}
static std::string protect(std::string const& s,bool decrypt){
#ifdef _WIN32
    DATA_BLOB in{static_cast<DWORD>(s.size()),reinterpret_cast<BYTE*>(const_cast<char*>(s.data()))},out{};
    BOOL ok=decrypt?CryptUnprotectData(&in,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out):CryptProtectData(&in,L"Geometry Intelligence chat history",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out);
    if(!ok)throw Error(ErrorCode::Storage,"Windows could not encrypt/decrypt history for this user.");
    std::string result(reinterpret_cast<char*>(out.pbData),out.cbData);SecureZeroMemory(out.pbData,out.cbData);LocalFree(out.pbData);return result;
#else
    (void)s;(void)decrypt;throw Error(ErrorCode::Storage,"Encrypted history requires Windows DPAPI.");
#endif
}
std::vector<Conversation> HistoryStore::load(){auto bytes=readBounded(file);if(bytes.empty())return {};try{auto j=Json::parse(protect(bytes,true));if(j.at("version")!=1||!j.at("conversations").is_array()||j["conversations"].size()>20)throw Error(ErrorCode::Storage,"Unsupported history format.");auto result=j["conversations"].get<std::vector<Conversation>>();for(auto const& c:result){if(c.messages.size()>80)throw Error(ErrorCode::Storage,"Invalid history length.");for(auto const& m:c.messages){if((m.role!="user"&&m.role!="assistant")||m.content.size()>50000||m.sources.size()>24)throw Error(ErrorCode::Storage,"Invalid message in history.");}}return result;}catch(Json::exception const&){throw Error(ErrorCode::Storage,"History is damaged; original file has been preserved.");}}
void HistoryStore::save(std::vector<Conversation> const& chats){auto data=Json{{"version",1},{"conversations",chats}}.dump(-1,' ',false,Json::error_handler_t::replace);if(data.size()>6*1024*1024)throw Error(ErrorCode::Storage,"History is full. Delete an old chat before saving more.");atomicWrite(file,protect(data,false));}
void HistoryStore::erase(){std::error_code ec;std::filesystem::remove(file,ec);if(ec)throw Error(ErrorCode::Storage,"Cannot delete history.");auto temp=file;temp+=".tmp";std::filesystem::remove(temp,ec);}
Cache::Cache(std::filesystem::path dir,bool on):file(std::move(dir)/"cache.json"),enabled(on){if(!enabled)return;try{auto s=readBounded(file,4*1024*1024);if(s.empty())return;auto j=Json::parse(s);if(j.value("version",0)!=1)return;if(j.contains("entries")&&j["entries"].is_object())entries=j["entries"];if(j.contains("snapshots")&&j["snapshots"].is_object())snapshots=j["snapshots"];for(auto it=entries.begin();it!=entries.end();){if(!it->is_object()||it->value("expires",int64_t{})<=epochNow())it=entries.erase(it);else ++it;}}catch(...){entries=Json::object();snapshots=Json::object();}}
std::optional<Source> Cache::get(std::string const& key){if(!enabled||!entries.contains(key))return {};auto const& item=entries[key];try{if(item.at("expires").get<int64_t>()<=epochNow()||item.at("created").get<int64_t>()>epochNow()+60)return {};auto s=item.at("source").get<Source>();if(!safeSourceUrl(s.url))return {};s.cached=true;return s;}catch(...){entries.erase(key);return {};}}
void Cache::flush(){if(!enabled)return;while(entries.size()>100)entries.erase(entries.begin());while(snapshots.size()>32)snapshots.erase(snapshots.begin());auto j=Json{{"version",1},{"entries",entries},{"snapshots",snapshots}};if(j.dump().size()>3*1024*1024){entries=Json::object();j["entries"]=entries;}atomicWrite(file,j.dump(-1,' ',false,Json::error_handler_t::replace));}
void Cache::put(std::string const& key,Source const& s,int ttl){if(!enabled||s.data.dump().size()>80000)return;entries[key]={{"source",s},{"created",epochNow()},{"expires",epochNow()+ttl}};flush();}
void Cache::observe(std::string const& key,Source const& s){if(!enabled||s.cached||s.data.dump().size()>80000)return;auto& h=snapshots[key];if(!h.is_array())h=Json::array();if(!h.empty()&&h.back().value("fetched_at","")==s.fetchedAt)return;h.push_back(s);while(h.size()>2)h.erase(h.begin());flush();}
Json Cache::history(std::string const& key)const{if(!enabled||!snapshots.contains(key)||!snapshots[key].is_array()||snapshots[key].size()<2)return {{"available",false},{"reason","Two local dated observations are required. This is not a global ranking archive."}};return {{"available",true},{"scope","Only two local observations; no conclusions about the period between them."},{"observations",snapshots[key]}};}
void Cache::clear(){entries=Json::object();snapshots=Json::object();std::error_code ec;std::filesystem::remove(file,ec);if(ec)throw Error(ErrorCode::Storage,"Cannot delete cache.");}
}
