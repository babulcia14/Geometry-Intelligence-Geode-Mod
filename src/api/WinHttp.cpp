#include <gi/Http.hpp>
#include <gi/Validation.hpp>
#include <windows.h>
#include <winhttp.h>
#include <array>
#include <atomic>
namespace gi {
namespace {
struct Handle{
    std::atomic<HINTERNET> h{};
    void close(){if(auto p=h.exchange(nullptr))WinHttpCloseHandle(p);}
    ~Handle(){close();}
    operator HINTERNET()const{return h.load();}
};
std::wstring wide(std::string const& s){int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),nullptr,0);std::wstring w(n,L'\0');MultiByteToWideChar(CP_UTF8,0,s.data(),static_cast<int>(s.size()),w.data(),n);return w;}
void check(BOOL ok){if(!ok){auto e=GetLastError();throw Error(e==ERROR_WINHTTP_TIMEOUT?ErrorCode::Timeout:ErrorCode::Network,"WinHTTP failed, code "+std::to_string(e)+".");}}
}
// Background-thread adapter, also used for streaming in the mod. Geode 5.11's
// web response API buffers the body and cannot deliver incremental SSE chunks.
Response WinHttp::perform(Request const& r,std::stop_token stop){
    checkCancelled(stop);auto u=parseUrl(r.url);
    Handle session{WinHttpOpen(L"GeometryIntelligence/1.0.0",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0)};check(session.h.load()!=nullptr);
    check(WinHttpSetTimeouts(session,5000,5000,r.timeout*1000,r.timeout*1000));
    auto host=wide(u.host);Handle connection{WinHttpConnect(session,host.c_str(),static_cast<INTERNET_PORT>(u.port),0)};check(connection.h.load()!=nullptr);
    auto method=wide(r.method),path=wide(u.path);Handle request{WinHttpOpenRequest(connection,method.c_str(),path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,u.scheme=="https"?WINHTTP_FLAG_SECURE:0)};check(request.h.load()!=nullptr);
    std::stop_callback cancel(stop,[&request]{request.close();});
    auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(r.timeout);
    DWORD redirects=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;check(WinHttpSetOption(request,WINHTTP_OPTION_REDIRECT_POLICY,&redirects,sizeof(redirects)));
    std::wstring headers;for(auto const& [k,v]:r.headers)headers+=wide(k+": "+v+"\r\n");
    check(WinHttpSendRequest(request,headers.c_str(),static_cast<DWORD>(headers.size()),r.body.empty()?WINHTTP_NO_REQUEST_DATA:const_cast<char*>(r.body.data()),static_cast<DWORD>(r.body.size()),static_cast<DWORD>(r.body.size()),0));
    checkCancelled(stop);check(WinHttpReceiveResponse(request,nullptr));
    Response out;DWORD status=0,size=sizeof(status);check(WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&size,WINHTTP_NO_HEADER_INDEX));out.status=static_cast<int>(status);
    std::array<wchar_t,128> retry{};size=static_cast<DWORD>(sizeof(retry));
    if(WinHttpQueryHeaders(request,WINHTTP_QUERY_CUSTOM,L"Retry-After",retry.data(),&size,WINHTTP_NO_HEADER_INDEX)){
        std::string value;
        for(size_t i=0;i<size/sizeof(wchar_t)&&i<retry.size()&&retry[i];++i){
            if(retry[i]>127){value.clear();break;}
            value.push_back(static_cast<char>(retry[i]));
        }
        out.headers["retry-after"]=value;
    }
    std::array<char,4096> buf{};DWORD count=0;for(;;){checkCancelled(stop);check(WinHttpReadData(request,buf.data(),static_cast<DWORD>(buf.size()),&count));if(count==0)break;out.body.append(buf.data(),count);if(out.body.size()>r.maxBytes)throw Error(ErrorCode::Limit,"HTTP response too large.");if(std::chrono::steady_clock::now()>deadline)throw Error(ErrorCode::Timeout,"Response deadline exceeded.");if(r.onChunk&&out.status>=200&&out.status<300)r.onChunk(std::string_view(buf.data(),count));}
    return out;
}
}
