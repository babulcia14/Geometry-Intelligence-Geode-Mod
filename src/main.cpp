#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/EditLevelLayer.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <gi/AssistantPopup.hpp>
#include <gi/GameContext.hpp>
#include <gi/Controller.hpp>
using namespace geode::prelude;

namespace {
void attach(CCNode* parent,CCPoint fallback,geode::Function<gi::Json()> context){
    auto* sprite=CCSprite::create("rub-logo.png"_spr);
    if(!sprite)return;
    sprite->setScale(40.f/sprite->getContentSize().width);
    auto* item=CCMenuItemExt::createSpriteExtra(sprite,[context=std::move(context)](auto*) mutable {if(auto* popup=gi::AssistantPopup::create(gi::readConfig().allowContext?context():gi::Json::object()))popup->show();});
    item->setID("open-assistant"_spr);
    if(auto* menu=parent->getChildByID("bottom-menu")){menu->addChild(item);menu->updateLayout();}
    else {auto* fallbackMenu=CCMenu::create();fallbackMenu->setID("assistant-menu"_spr);fallbackMenu->setPosition(fallback);fallbackMenu->addChild(item);parent->addChild(fallbackMenu,100);}
}
}
class $modify(GIMenu,MenuLayer){
    bool init(){if(!MenuLayer::init())return false;attach(this,{32,50},[]{return gi::Json::object();});return true;}
};
class $modify(GIPause,PauseLayer){
    void customSetup(){PauseLayer::customSetup();attach(this,{32,50},[]{auto* p=PlayLayer::get();return gi::captureLevel(p?p->m_level:nullptr,"pause");});}
};
class $modify(GIEdit,EditLevelLayer){
    bool init(GJGameLevel* level){if(!EditLevelLayer::init(level))return false;attach(this,{32,50},[this]{return gi::captureLevel(m_level,"editor-level-menu");});return true;}
};
