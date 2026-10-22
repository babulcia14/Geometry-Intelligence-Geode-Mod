#include <gi/Knowledge.hpp>
#include <gi/Store.hpp>
#include <gi/Text.hpp>
#include <gi/Validation.hpp>
#include <algorithm>
namespace gi {
namespace {
std::string fold(std::string text){
    for(auto const& pair:std::vector<std::pair<std::string,std::string>>{{"ą","a"},{"Ą","a"},{"ć","c"},{"Ć","c"},{"ę","e"},{"Ę","e"},{"ł","l"},{"Ł","l"},{"ń","n"},{"Ń","n"},{"ó","o"},{"Ó","o"},{"ś","s"},{"Ś","s"},{"ź","z"},{"Ź","z"},{"ż","z"},{"Ż","z"}}){
        for(size_t at=0;(at=text.find(pair.first,at))!=std::string::npos;at+=pair.second.size())text.replace(at,pair.first.size(),pair.second);
    }
    return lower(std::move(text));
}
}
Knowledge::Knowledge(std::filesystem::path const& path){auto raw=readBounded(path,1024*1024);if(raw.empty())throw Error(ErrorCode::Storage,"Local knowledge resource is missing.");documents=parseJson(raw);if(!documents.is_array())throw Error(ErrorCode::InvalidData,"Invalid local knowledge base.");}
std::vector<Source> Knowledge::search(std::string const& q,size_t limit,std::string const& language)const{
    auto query=fold(q);auto words=split(query," ");std::vector<std::pair<int,size_t>> ranked;
    for(size_t i=0;i<documents.size();++i){auto const& d=documents[i];int score=0;for(auto const& tag:d.at("keywords")){auto word=fold(tag.get<std::string>());if(query.find(word)!=std::string::npos)score+=10;}auto hay=fold(d.at("facts").dump()+d.value("facts_pl",Json::array()).dump());for(auto const& w:words)if(w.size()>3&&hay.find(w)!=hay.npos)++score;if(score)ranked.emplace_back(score,i);}
    std::stable_sort(ranked.begin(),ranked.end(),[](auto const& a,auto const& b){return a.first>b.first;});
    std::vector<Source> out;
    for(auto const& [score,i]:ranked){if(!ranked.empty()&&ranked[0].first>=10&&score<5)continue;auto const& d=documents[i];auto url=requiredString(d,"url",2048);if(!safeSourceUrl(url))continue;out.push_back({requiredString(d,"title"),url,d.value("checked","2026-10-20"),"local-reference-notes",true,{{"facts",language=="pl"&&d.contains("facts_pl")?d.at("facts_pl"):d.at("facts")},{"scope",d.value("scope","GD 2.2. These are reference facts, not a canned chat response.")}}});if(out.size()>=limit)break;}
    return out;
}
}
