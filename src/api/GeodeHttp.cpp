#include <gi/Http.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/async.hpp>
#include <future>
#include <atomic>
namespace gi {
// Called ONLY by Controller's worker. No game/UI pointers cross into this adapter.
Response GeodeHttp::perform(Request const& req,std::stop_token stop){
    using namespace geode;
    checkCancelled(stop);
    // Geode 5.11's web API exposes progress but not body chunks. WinHTTP
    // streams on the same background worker, with cancellable handles.
    if(req.onChunk){WinHttp stream;return stream.perform(req,stop);}
    auto promise=std::make_shared<std::promise<Response>>();auto future=promise->get_future();
    auto overflow=std::make_shared<std::atomic_bool>(false);
    utils::web::WebRequest webRequest;
    webRequest.timeout(std::chrono::seconds(req.timeout)).followRedirects(false).certVerification(true)
        .userAgent("GeometryIntelligence/1.0.0")
        .onProgress([overflow,limit=req.maxBytes](auto const& p){if(p.downloaded()>limit||p.downloadTotal()>limit)overflow->store(true);});
    for(auto const& [key,value]:req.headers)webRequest.header(key,value);
    if(!req.body.empty())webRequest.bodyString(req.body);
    auto task=async::spawn([request=std::move(webRequest),req,promise]()mutable->arc::Future<> {
        try {auto result=co_await request.send(req.method,req.url);
            Response out;out.status=result.code();
            if(out.status<0)throw Error(out.status==-28?ErrorCode::Timeout:ErrorCode::Network,"Network request failed (DNS, TLS, connection or timeout). Code "+std::to_string(out.status)+".");
            auto text=result.string();if(!text)throw Error(ErrorCode::Network,"Could not read HTTP response.");out.body=std::move(text).unwrap();
            if(auto retry=result.header("Retry-After"))out.headers["retry-after"]=std::string(*retry);
            promise->set_value(std::move(out));
        }catch(...){promise->set_exception(std::current_exception());}
        co_return;
    });
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(req.timeout+2);
    while(future.wait_for(std::chrono::milliseconds(25))!=std::future_status::ready){
        if(stop.stop_requested()||overflow->load()||std::chrono::steady_clock::now()>deadline){
            task.abort();checkCancelled(stop);
            if(overflow->load())throw Error(ErrorCode::Limit,"Response exceeds configured size limit.");
            throw Error(ErrorCode::Timeout,"Request timed out.");
        }
    }
    checkCancelled(stop);if(overflow->load())throw Error(ErrorCode::Limit,"Response exceeds configured size limit.");return future.get();
}
}
