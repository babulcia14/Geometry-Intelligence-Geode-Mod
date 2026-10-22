#include <gi/AssistantPopup.hpp>
#include <gi/Controller.hpp>
#include <gi/Text.hpp>
#include <gi/Validation.hpp>
#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/utils/web.hpp>
using namespace geode::prelude;

namespace gi {
namespace {
char const* font(){return "gi-font.fnt"_spr;}
std::string tr(char const* pl,char const* en){return readConfig().language=="pl"?pl:en;}
CCLabelBMFont* label(std::string const& text,float scale,float width=0){
    auto* l=CCLabelBMFont::create(text.c_str(),font(),width>0?width/scale:kCCLabelAutomaticWidth,kCCTextAlignmentLeft);
    l->setScale(scale);l->setColor({235,240,249});return l;
}
CCMenuItemSpriteExtra* button(CCMenu* menu,std::string const& text,float x,float y,float width,geode::Function<void()> action){
    auto* bg=NineSlice::create("GJ_square02.png");bg->setContentSize({width,24});bg->setColor({35,49,68});
    auto* title=label(text,.43f);title->limitLabelWidth(width-8,.43f,.24f);title->setPosition({width/2,12});bg->addChild(title);
    auto* b=CCMenuItemExt::createSpriteExtra(bg,[action=std::move(action)](auto*) mutable {action();});
    b->setID("gi-"+text);b->setPosition({x,y});menu->addChild(b);return b;
}
class SecretPopup final:public Popup {
    TextInput* field{};
    std::string target;
    bool setup(bool search){
        if(!Popup::init(380,174,"GJ_square02.png"))return false;
        m_bgSprite->setColor({20,25,35});setTitle(search?"Optional search API key":"Optional model API key",font(),.6f);
        target=search?Credentials::searchTarget:Credentials::modelTarget(readConfig().endpoint);
        auto* note=label(tr("Własny lokalny model nie potrzebuje klucza.\nPuste pole + Zapisz usuwa zapisany klucz.","Our local model needs no key.\nEmpty field + Save removes the saved key."),.43f,340);note->setPosition({190,108});m_mainLayer->addChild(note);
        field=TextInput::create(330,"API key",font());field->setPasswordMode(true);field->setCommonFilter(CommonFilter::Any);field->setMaxCharCount(2300);field->setPosition({190,65});m_mainLayer->addChild(field);
        button(m_buttonMenu,tr("Zapisz","Save"),190,25,100,[this]{controller().secret(target,std::string(field->getString()));field->setString("");onClose(nullptr);});
        return true;
    }
public:
    static SecretPopup* create(bool search){auto* p=new SecretPopup;if(p->setup(search)){p->autorelease();return p;}delete p;return nullptr;}
};
}
AssistantPopup* AssistantPopup::create(Json context){auto* p=new AssistantPopup;if(p->setup(std::move(context))){p->autorelease();return p;}delete p;return nullptr;}
bool AssistantPopup::setup(Json context){
    auto win=CCDirector::sharedDirector()->getWinSize();
    float w=std::min(540.f,win.width-20),h=std::min(320.f,win.height-14);
    if(!Popup::init(w,h,"GJ_square02.png"))return false;
    setID("assistant-popup"_spr);game=std::move(context);
    auto theme=Mod::get()->getSettingValue<std::string>("theme");m_bgSprite->setColor(theme=="midnight"?ccColor3B{10,18,32}:ccColor3B{23,27,35});
    auto* title=label("Geometry Intelligence: Rub",.60f);title->limitLabelWidth(w-125,.60f,.35f);title->setAnchorPoint({0,1});title->setPosition({16,h-12});m_mainLayer->addChild(title);
    auto* subtitle=label(tr("Rub - Twój asystent AI w Geometry Dash","Rub - Your AI Companion for Geometry Dash"),.35f);subtitle->setColor({117,162,224});subtitle->setAnchorPoint({0,1});subtitle->setPosition({17,h-33});m_mainLayer->addChild(subtitle);
    sidebar=CCMenu::create();sidebar->setPosition({0,0});sidebar->setID("sidebar"_spr);m_mainLayer->addChild(sidebar);
    button(sidebar,tr("Nowa rozmowa","New Chat"),65,h-69,100,[this]{controller().newChat(readConfig().saveHistory);historyMode=false;messagePage=0;seen=~uint64_t{};});
    button(sidebar,tr("Historia","Chat History"),65,h-99,100,[this]{historyMode=!historyMode;seen=~uint64_t{};});
    button(sidebar,tr("Szukaj","Search"),65,h-129,100,[this]{input->setString(tr("Jaki jest obecnie Top 1 Demon?","What is the current Top 1 Demon?"));input->focus();});
    button(sidebar,tr("Ustawienia","Settings"),65,h-159,100,[]{openSettingsPopup(Mod::get());});
    button(sidebar,tr("Połączenie","Connection"),65,h-189,100,[]{controller().connection(readConfig());});
    button(sidebar,tr("Klucz modelu","Model API key"),65,h-219,100,[]{if(auto* p=SecretPopup::create(false))p->show();});
    button(sidebar,tr("Klucz szukania","Search API key"),65,h-249,100,[]{if(auto* p=SecretPopup::create(true))p->show();});
    button(m_buttonMenu,"PL / EN",w-57,h-18,80,[this]{
        if(controller().snapshot(false).busy){
            Notification::create(tr("Najpierw zatrzymaj odpowiedź.","Stop the response first."),NotificationIcon::Info)->show();return;
        }
        createQuickPopup("Language / Język","Wybierz język interfejsu i odpowiedzi.\nChoose the interface and response language.","Polski","English",[this](auto*,bool english){
            Mod::get()->setSettingValue<std::string>("language",english?"en":"pl");
            auto* next=AssistantPopup::create(game);
            if(next){next->input->setString(std::string(input->getString()));next->show();onClose(nullptr);}
        });
    });
    paneWidth=w-143;paneHeight=h-119;
    auto* panel=NineSlice::create("GJ_square02.png");panel->setColor({13,18,25});panel->setContentSize({paneWidth,paneHeight});panel->setAnchorPoint({0,0});panel->setPosition({128,65});m_mainLayer->addChild(panel);
    chatScroll=ScrollLayer::create(CCSize{paneWidth-10,paneHeight-10});chatScroll->setPosition({133,70});chatScroll->setID("chat-scroll"_spr);m_mainLayer->addChild(chatScroll);
    input=TextInput::create(paneWidth-75,tr("Napisz wiadomość...","Write a message..."),font());input->setCommonFilter(CommonFilter::Any);
    input->setFilter("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,:;!?'-_+=()[]{}<>/\\@#$%&*\"ąćęłńóśźżĄĆĘŁŃÓŚŹŻ");
    input->setMaxCharCount(2000);input->setTextAlign(TextInputAlign::Left);input->setPosition({128+(paneWidth-75)/2,45});input->setID("chat-input"_spr);m_mainLayer->addChild(input);
    button(m_buttonMenu,tr("Wyślij","Send"),w-47,45,64,[this]{send();});
    button(m_buttonMenu,tr("Stop","Stop"),w-47,17,64,[]{controller().stop();});
    button(m_buttonMenu,tr("Kopiuj","Copy"),165,17,70,[]{auto v=controller().snapshot();if(v.chats.empty())return;auto const& messages=v.chats[v.active].messages;for(auto i=messages.rbegin();i!=messages.rend();++i)if(i->role=="assistant"){std::string text=i->content;for(auto const& s:i->sources)text+="\n"+s.title+" "+s.url+" "+s.fetchedAt;clipboard::write(text);break;}});
    button(m_buttonMenu,tr("Usuń czat","Delete chat"),242,17,78,[]{
        createQuickPopup("Geometry Intelligence: Rub",tr("Usunąć bieżącą rozmowę?","Delete the current conversation?"),"Cancel","Delete",[](auto*,bool yes){if(yes)controller().removeChat(readConfig().saveHistory);});
    });
    button(m_buttonMenu,tr("Wyczyść","Clear data"),326,17,78,[]{
        createQuickPopup("Geometry Intelligence: Rub",tr("Usunąć całą historię i cache? Klucze usuwa się osobno.","Delete all history and cache? API keys are removed separately."),"Cancel","Delete",[](auto*,bool yes){if(yes)controller().clear(true);});
    });
    status=label("",.33f);status->setAnchorPoint({0,1});status->setPosition({128,h-36});status->setColor({124,176,239});m_mainLayer->addChild(status);
    modelLabel=label("",.30f);modelLabel->setAnchorPoint({0,0});modelLabel->setPosition({16,8});modelLabel->setColor({136,155,177});m_mainLayer->addChild(modelLabel);
    if(controller().snapshot().chats.empty())controller().load(readConfig().saveHistory);
    schedule(schedule_selector(AssistantPopup::tick),.25f);
    tick(0);return true;
}
void AssistantPopup::send(){if(controller().snapshot(false).busy)return;auto text=std::string(input->getString());if(trim(text).empty())return;auto c=readConfig();controller().ask(text,c.allowContext?game:Json::object(),c);input->setString("");input->defocus();historyMode=false;messagePage=0;seen=~uint64_t{};}
void AssistantPopup::tick(float){auto v=controller().snapshot(false);if(wasBusy&&!v.busy&&v.error.empty()&&Mod::get()->getSettingValue<bool>("notifications"))Notification::create(tr("Odpowiedź gotowa","Response ready"),NotificationIcon::Success)->show();wasBusy=v.busy;
    status->setString(clip(v.status,80).c_str());status->limitLabelWidth(paneWidth,.33f,.22f);modelLabel->setString(clip(readConfig().model,22).c_str());
    input->setEnabled(!v.busy);if(seen!=v.revision){seen=v.revision;render();}
}
void AssistantPopup::render(){
    auto v=controller().snapshot();auto* content=chatScroll->m_contentLayer;content->removeAllChildrenWithCleanup(true);
    auto* menu=CCMenu::create();menu->setPosition({0,0});content->addChild(menu);
    std::vector<std::pair<CCNode*,float>> items;float total=8;const float width=paneWidth-24;
    auto text=[&](std::string const& value,float scale,ccColor3B color){auto* l=label(value,scale,width);l->setColor(color);l->setAnchorPoint({0,1});content->addChild(l);float height=l->getScaledContentSize().height+8;items.emplace_back(l,height);total+=height;};
    if(historyMode){
        text(tr("Historia rozmów","Chat history"),.55f,{120,177,249});
        for(size_t i=0;i<v.chats.size();++i){auto* b=button(menu,clip(v.chats[i].title,70),width/2,0,width,[this,i]{controller().select(i);historyMode=false;messagePage=0;seen=~uint64_t{};});items.emplace_back(b,31);total+=31;}
    }else if(v.chats.empty()||v.chats[v.active].messages.empty()){
        text(tr("Twój własny model. Twoje pytania.","Your own model. Your questions."),.65f,{236,242,252});
        text(tr("Uruchom lokalny serwer Python, potem sprawdź Połączenie. Model gi-tiny jest trenowany od zera i ma ograniczone możliwości rozmowy.","Start the local Python server, then check Connection. The gi-tiny model is trained from scratch and has limited conversation ability."),.46f,{176,192,212});
        text(tr("Do pobierania rankingów włącz Public APIs w Ustawieniach. Źródła i daty pojawią się pod odpowiedzią.","Enable Public APIs in Settings for live rankings. Sources and retrieval dates appear below answers."),.43f,{145,166,194});
    }else{
        auto const& messages=v.chats[v.active].messages;
        messagePage=std::min(messagePage,(messages.size()-1)/12);
        size_t end=messages.size()-messagePage*12,begin=end>12?end-12:0;
        if(begin){auto* b=button(menu,tr("Starsze wiadomości","Older messages"),width/2,0,width,[this]{++messagePage;seen=~uint64_t{};});items.emplace_back(b,31);total+=31;}
        if(messagePage){auto* b=button(menu,tr("Nowsze wiadomości","Newer messages"),width/2,0,width,[this]{--messagePage;seen=~uint64_t{};});items.emplace_back(b,31);total+=31;}
        for(size_t i=begin;i<end;++i){auto const& m=messages[i];text(m.role=="user"?tr("TY","YOU"):"Rub",.36f,m.role=="user"?ccColor3B{149,184,233}:ccColor3B{112,195,239});
            text(clip(m.content,12000),.46f,{233,239,248});
            for(size_t n=0;n<m.sources.size();++n){auto const& s=m.sources[n];std::string line="["+std::to_string(n+1)+"] "+s.title+"\n"+s.fetchedAt+(s.cached?" (cache)":"");text(line,.33f,{131,162,201});
                if(safeSourceUrl(s.url)){auto url=s.url;auto* b=button(menu,tr("Otwórz źródło","Open source"),width/2,0,width,[url]{web::openLinkInBrowser(url);});items.emplace_back(b,29);total+=29;}
            }
        }
    }
    if(!v.partial.empty())text(clip(v.partial,12000),.46f,{184,209,242});
    if(!v.error.empty())text(v.error,.43f,{245,176,146});
    if(v.busy)text(v.status,.38f,{132,174,228});
    total=std::max(total,paneHeight-10);content->setContentSize({paneWidth-10,total});
    float y=total-5;for(auto const& [node,height]:items){if(typeinfo_cast<CCMenuItemSpriteExtra*>(node))node->setPosition({width/2+5,y-height/2});else node->setPosition({5,y});y-=height;}
    if(historyMode||messagePage)chatScroll->scrollToTop();else content->setPositionY(0);
}
void AssistantPopup::onClose(CCObject* sender){unschedule(schedule_selector(AssistantPopup::tick));controller().stop();Popup::onClose(sender);}
}
