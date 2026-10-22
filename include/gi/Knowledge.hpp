#pragma once
#include "Types.hpp"
namespace gi {
class Knowledge {
    Json documents;
public:
    explicit Knowledge(std::filesystem::path const& file);
    std::vector<Source> search(std::string const& query,size_t limit=4,std::string const& language="en") const;
};
}
