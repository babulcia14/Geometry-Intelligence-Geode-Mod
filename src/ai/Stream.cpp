#include <gi/Stream.hpp>
#include <gi/Validation.hpp>

namespace gi {
void ChatStream::feed(std::string_view chunk) {
    bytes += chunk.size();
    if(bytes > 2*1024*1024) throw Error(ErrorCode::Limit,"Model stream exceeds 2 MiB.");
    pending.append(chunk);
    for(size_t end; (end=pending.find('\n')) != std::string::npos;) {
        auto next=pending.substr(0,end);
        pending.erase(0,end+1);
        if(!next.empty()&&next.back()=='\r')next.pop_back();
        line(next);
    }
}
void ChatStream::line(std::string const& line) {
    if(line.empty()||line[0]==':'||!line.starts_with("data:"))return;
    auto data=line.substr(5);if(!data.empty()&&data[0]==' ')data.erase(0,1);
    if(data=="[DONE]"){done=true;return;}
    if(done)throw Error(ErrorCode::InvalidData,"Data after stream terminator.");
    auto event=parseJson(data);
    if(event.contains("error"))throw Error(ErrorCode::Model,"Model returned a streaming error.");
    if(!event.contains("choices")||!event["choices"].is_array())throw Error(ErrorCode::InvalidData,"Invalid model stream choices.");
    if(event["choices"].empty())return; // usage-only final chunk
    auto const& choice=event["choices"][0];
    if(choice.contains("finish_reason")&&!choice["finish_reason"].is_null()){
        reason=requiredString(choice,"finish_reason",100);finished=true;
    }
    if(!choice.contains("delta")||!choice["delta"].is_object())throw Error(ErrorCode::InvalidData,"Invalid model stream delta.");
    auto const& d=choice["delta"];
    if(d.contains("content")&&!d["content"].is_null()){
        if(!d["content"].is_string())throw Error(ErrorCode::InvalidData,"Non-text content in stream.");
        content+=d["content"].get<std::string>();
        if(content.size()>50000)throw Error(ErrorCode::Limit,"Model text is too long.");
        if(onText)onText(content);
    }
    if(d.contains("tool_calls")){
        if(!d["tool_calls"].is_array())throw Error(ErrorCode::InvalidData,"Invalid streamed tool calls.");
        for(auto const& call:d["tool_calls"]){
            int index=boundedInt(call,"index",-1,0,5);
            if(index<0)throw Error(ErrorCode::InvalidData,"Missing streamed tool index.");
            auto& out=calls[index];
            if(out.is_null())out={{"id",""},{"type","function"},{"function",{{"name",""},{"arguments",""}}}};
            if(call.contains("id"))out["id"]=out["id"].get<std::string>()+requiredString(call,"id",200);
            if(call.contains("function")){
                auto const& f=call["function"];
                if(!f.is_object())throw Error(ErrorCode::InvalidData,"Invalid function delta.");
                for(auto const* key:{"name","arguments"})if(f.contains(key)){
                    if(!f[key].is_string())throw Error(ErrorCode::InvalidData,"Invalid function field delta.");
                    out["function"][key]=out["function"][key].get<std::string>()+f[key].get<std::string>();
                    if(out["function"][key].get_ref<std::string const&>().size()>4096)throw Error(ErrorCode::Limit,"Streamed tool arguments too long.");
                }
            }
        }
    }
}
Json ChatStream::response(){
    if(!pending.empty()){line(pending);pending.clear();}
    if(!done||!finished)throw Error(ErrorCode::Model,"Incomplete model stream; response was interrupted.");
    Json m={{"role","assistant"},{"content",content}};
    if(!calls.empty()){
        m["tool_calls"]=Json::array();
        for(auto const& [index,call]:calls){(void)index;m["tool_calls"].push_back(call);}
    }
    return {{"choices",Json::array({{{"message",m},{"finish_reason",reason}}})}};
}
}
