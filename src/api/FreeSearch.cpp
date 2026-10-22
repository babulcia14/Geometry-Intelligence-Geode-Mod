#include <gi/Api.hpp>
#include <gi/Text.hpp>
#include <gi/Validation.hpp>
#include <regex>

namespace gi {
Source Api::freeSearch(std::string const& query,std::string const& scope,std::stop_token stop){
    if(scope=="music"){
        auto url="https://musicbrainz.org/ws/2/recording/?query="+encode(query)+"&fmt=json&limit=6&dismax=true";
        auto result=fetch("MusicBrainz catalogue (not a GD whitelist)",url,600,stop,[](Json const& j){
            if(!j.is_object()||!j.contains("recordings")||!j["recordings"].is_array())throw Error(ErrorCode::InvalidData,"Invalid MusicBrainz response.");
            Json out=Json::array();
            for(auto const& row:j["recordings"]){
                auto id=requiredString(row,"id",36);if(!std::regex_match(id,std::regex("[a-f0-9-]{36}")))continue;
                std::string artist;
                if(row.contains("artist-credit")&&row["artist-credit"].is_array())for(auto const& c:row["artist-credit"]){
                    if(c.is_object()&&c.contains("name")&&c["name"].is_string()){if(!artist.empty())artist+=", ";artist+=c["name"].get<std::string>();}
                }
                out.push_back({{"title",requiredString(row,"title",1000)},{"author",artist.empty()?Json(nullptr):Json(artist)},
                    {"url","https://musicbrainz.org/recording/"+id},{"song_id",nullptr},{"genre",nullptr},{"mood",nullptr},
                    {"catalogue_tags",row.value("tags",Json::array())},{"gd_available","unverified"},{"licence","not_verified"}});
            }
            return Json{{"results",out},{"note","Free metadata catalogue. This is NOT Newgrounds search, a GD whitelist or a royalty-free music library. Song IDs are absent; genre/mood are unknown unless supported by supplied tags. No audio is analysed or downloaded. Obtain rights and verify GD availability independently."}};
        });result.confidence="community-music-metadata";return result;
    }
    auto url="https://en.wikipedia.org/w/api.php?action=query&list=search&srsearch="+encode("Geometry Dash "+query)+"&srlimit=5&srprop=snippet%7Ctimestamp&format=json";
    auto result=fetch("Wikipedia search (limited free web source)",url,300,stop,[](Json const& j){
        if(!j.is_object()||!j.contains("query")||!j["query"].contains("search")||!j["query"]["search"].is_array())throw Error(ErrorCode::InvalidData,"Invalid Wikipedia search response.");
        Json out=Json::array();for(auto const& row:j["query"]["search"]){auto title=requiredString(row,"title",500);
            std::string snippet=row.value("snippet","");snippet=std::regex_replace(snippet,std::regex("<[^>]*>"),"");
            out.push_back({{"title",title},{"url","https://en.wikipedia.org/wiki/"+encode(title)},{"snippet",clip(plainText(snippet),2000)},{"page_timestamp",row.value("timestamp","")}});
        }
        return Json{{"results",out},{"note","This free search covers Wikipedia only. Snippets may be incomplete or outdated. It is not comprehensive web search and does not verify current rankings or all community claims. Text attribution: Wikipedia contributors, CC BY-SA; follow the page URL for history."}};
    });result.confidence="encyclopedia-search-snippets";return result;
}
}
