#pragma once
#include "Types.hpp"
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/TextInput.hpp>

namespace gi {
class AssistantPopup : public geode::Popup {
    Json game;
    geode::TextInput* input=nullptr;
    geode::ScrollLayer* chatScroll=nullptr;
    cocos2d::CCLabelBMFont* status=nullptr;
    cocos2d::CCLabelBMFont* modelLabel=nullptr;
    cocos2d::CCMenu* sidebar=nullptr;
    uint64_t seen=~uint64_t{};
    bool historyMode=false,wasBusy=false;
    size_t messagePage=0;
    float paneWidth=0,paneHeight=0;
    bool setup(Json);
    void tick(float);
    void render();
    void send();
    void onClose(cocos2d::CCObject*) override;
public:
    static AssistantPopup* create(Json game=Json::object());
};
}
