#pragma once
#include "Types.hpp"
#include <mutex>
namespace gi {
void atomicWrite(std::filesystem::path const&,std::string const&);
std::string readBounded(std::filesystem::path const&,size_t limit=8*1024*1024);
class HistoryStore {
    std::filesystem::path file;
public:
    explicit HistoryStore(std::filesystem::path dir):file(std::move(dir)/"history.dpapi"){}
    std::vector<Conversation> load();
    void save(std::vector<Conversation> const&);
    void erase();
};
class Cache {
    std::filesystem::path file;
    Json entries=Json::object(), snapshots=Json::object();
    bool enabled;
    void flush();
public:
    Cache(std::filesystem::path dir,bool enabled);
    std::optional<Source> get(std::string const& key);
    void put(std::string const& key,Source const&,int ttlSeconds);
    void observe(std::string const& key,Source const&);
    Json history(std::string const& key) const;
    void clear();
};
class Credentials {
public:
    static std::string modelTarget(std::string const& endpoint);
    static constexpr char const* searchTarget="GeometryIntelligence/BraveSearch";
    static std::string read(std::string const& target);
    static void write(std::string const& target,std::string const& secret);
    static void erase(std::string const& target);
};
}
