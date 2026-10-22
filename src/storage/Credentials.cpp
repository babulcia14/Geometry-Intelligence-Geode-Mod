#include <gi/Store.hpp>
#include <gi/Validation.hpp>
#ifdef _WIN32
#include <windows.h>
#include <wincred.h>
#endif
namespace gi {
std::string Credentials::modelTarget(std::string const& endpoint){return "GeometryIntelligence/model/"+validateEndpoint(endpoint,true);}
#ifdef _WIN32
static std::wstring wide(std::string const& s){int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),nullptr,0);if(!n&&!s.empty())throw Error(ErrorCode::Credentials,"Invalid credential name.");std::wstring w(n,L'\0');MultiByteToWideChar(CP_UTF8,0,s.data(),static_cast<int>(s.size()),w.data(),n);return w;}
#endif
std::string Credentials::read(std::string const& target){
#ifdef _WIN32
    PCREDENTIALW p=nullptr;auto w=wide(target);if(!CredReadW(w.c_str(),CRED_TYPE_GENERIC,0,&p)){if(GetLastError()==ERROR_NOT_FOUND)return {};throw Error(ErrorCode::Credentials,"Windows Credential Manager is unavailable.");}
    std::string result(reinterpret_cast<char*>(p->CredentialBlob),p->CredentialBlobSize);CredFree(p);return result;
#else
    (void)target;return {};
#endif
}
void Credentials::write(std::string const& target,std::string const& key){
    if(key.empty()||key.size()>2400||key.find_first_of("\r\n\0",0,3)!=std::string::npos)throw Error(ErrorCode::Credentials,"Invalid API key.");
#ifdef _WIN32
    auto w=wide(target);CREDENTIALW c{};c.Type=CRED_TYPE_GENERIC;c.TargetName=w.data();c.CredentialBlobSize=static_cast<DWORD>(key.size());c.CredentialBlob=reinterpret_cast<BYTE*>(const_cast<char*>(key.data()));c.Persist=CRED_PERSIST_LOCAL_MACHINE;c.UserName=const_cast<wchar_t*>(L"Geometry Intelligence");if(!CredWriteW(&c,0))throw Error(ErrorCode::Credentials,"Cannot store key in Windows Credential Manager.");
#else
    (void)target;throw Error(ErrorCode::Credentials,"Credential Manager requires Windows.");
#endif
}
void Credentials::erase(std::string const& target){
#ifdef _WIN32
    auto w=wide(target);if(!CredDeleteW(w.c_str(),CRED_TYPE_GENERIC,0)&&GetLastError()!=ERROR_NOT_FOUND)throw Error(ErrorCode::Credentials,"Cannot remove credential.");
#else
    (void)target;
#endif
}
}
