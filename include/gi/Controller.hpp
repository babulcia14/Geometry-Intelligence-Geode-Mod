#pragma once
#include "Engine.hpp"
#include <thread>

namespace gi {
struct View {
    std::vector<Conversation> chats;
    size_t active = 0;
    bool busy = false;
    std::string status = "Ready", partial, error;
    uint64_t revision = 0;
};

// No Cocos nodes in this class. The popup polls value snapshots; a worker never
// calls a UI callback, so closing a popup cannot leave a dangling UI pointer.
class Controller {
    struct State {std::mutex mutex;View view;std::stop_source stop;};
    std::shared_ptr<State> state = std::make_shared<State>();
    std::filesystem::path saveDir, knowledgeFile;
    using Work = std::function<void(View&,std::stop_token,Progress const&,Stream const&)>;
    bool launch(Work);
    static void persist(View const&, std::filesystem::path const&, bool);
public:
    Controller(std::filesystem::path save, std::filesystem::path knowledge);
    View snapshot(bool includeMessages = true) const;
    void stop();
    void load(bool saveHistory);
    void ask(std::string question, Json context, Config config);
    void connection(Config config);
    void newChat(bool saveHistory);
    void select(size_t index);
    void removeChat(bool saveHistory);
    void clear(bool includeCache);
    void secret(std::string target,std::string value);
};
Config readConfig();
Controller& controller();
}
