#pragma once
#include "Types.hpp"
#include <mutex>
namespace gi {
struct Request {
    std::string method="GET",url,body;
    std::map<std::string,std::string> headers;
    int timeout=20;
    size_t maxBytes=2*1024*1024;
    // Optional raw streaming callback, called on a worker, never on the game thread.
    std::function<void(std::string_view)> onChunk;
};
struct Response {int status=0;std::string body;std::map<std::string,std::string> headers;};
class Transport {public:virtual ~Transport()=default;virtual Response perform(Request const&,std::stop_token)=0;};
class Http {
    Transport& transport;
public:
    explicit Http(Transport& t):transport(t){}
    Response request(Request const&,std::stop_token);
    Json json(Request const&,std::stop_token);
};
class GeodeHttp final:public Transport {public:Response perform(Request const&,std::stop_token) override;};
class WinHttp final:public Transport {public:Response perform(Request const&,std::stop_token) override;};
}
