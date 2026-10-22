#include <gi/GameContext.hpp>
#include <gi/Text.hpp>
#include <Geode/binding/GJGameLevel.hpp>

namespace gi {
Json captureLevel(GJGameLevel* level,char const* screen){
    if(!level)return {};
    // Called on the main thread. No account data, saves, level string, objects,
    // input state or private messages are read. The worker receives a copy.
    return {{"screen",screen},{"captured_at",utcNow()},{"level_id",static_cast<int>(level->m_levelID)},
        {"name",clip(std::string(level->m_levelName),120)},
        {"creator",clip(std::string(level->m_creatorName),100)},
        {"song_id",level->m_songID},{"stars",static_cast<int>(level->m_stars)},
        {"source","Visible game metadata; not a live server verification"}};
}
}
