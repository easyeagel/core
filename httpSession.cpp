//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  httpSession.cpp
//
//   Description:  http 会话实现
//
//       Version:  1.0
//       Created:  2015年03月03日 16时10分04秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<core/sessionPool.hpp>

#include"httpSession.hpp"

namespace ezweb
{


HttpSSession::HttpSSession(AsyncStream&& hp)
    :BaseThis(std::move(hp))
    , coroutine_([this](core::CoroutineContext::CallFun&& call)
        {
            ioserviceGet().post(std::move((call)));
        }
    )
{}

void HttpSSession::sessionStart()
{
    auto self=this->shared_from_this();
    coroutine_.start([self, this]()
        {
            loop();
            auto s = std::move(const_cast<std::shared_ptr<core::SSession>&>(self));
            core::MainServer::post([s]()
                {
                    auto tself=std::move(s);
                    tself->sessionShutDown();
                }
            );
        }
    );
}

void HttpSSession::loop()
{
    for(;;)
    {
        read(coroutine_);
        if(bad())
            return;

        httpParser_.parse(buffer_.data(), lastBufferSize_);

        if(httpParser_.bad())
            return;

        if(!httpParser_.isMessageComplete())
            continue;

        switch(httpParser_.methodGet())
        {
            case HTTP_GET:
                httpGet();
                if(bad())
                    return;
                break;
            default:
                return;
        }

        if(httpParser_.isKeep() && close_==false)
            continue;

        return;
    }
}

void HttpSSession::httpGet()
{
    auto& hg=HttpGetDispatch::instance();

    GMacroSessionLog(*this, core::SeverityLevel::info)
        << "httpGet:" << httpParser_.hostGet() << httpParser_.pathGet();

    auto respones=hg.dispatch(this->ecGet(), httpParser_);

    write(coroutine_, *respones);

    close_=true;
}

void HttpSSession::sessionShutDown()
{
    BaseThis::sessionShutDown();
    core::SessionManager::free(this->shared_from_this());
}

HttpCSession::HttpCSession()
    :httpRequest_(HTTP_GET)
{
    httpParser_.onBody([this](const char* b, size_t n)
        {
            return contentCopy(b, n);
        }
    );
}

std::string& HttpCSession::urlGet(core::CoroutineContext& cc, const std::string& url)
{
    GMacroSessionLog(*this, core::SeverityLevel::info)
        << "urlGet:" << url;

    http_parser_url token;
    http_parser_parse_url(url.c_str(), url.size(), 1, &token);

    httpRequest_.reset(HTTP_GET, url.substr(token.field_data[UF_PATH].off));
    httpRequest_.commonHeadSet("Host", url.substr(token.field_data[UF_HOST].off, token.field_data[UF_HOST].len));
    httpRequest_.commonHeadSet("Connection", "Keep-Alive");

    httpRequest_.cache();

    content_.clear();
    write(cc, httpRequest_);
    if(this->bad())
        return content_;

    httpParser_.reset();
    while(httpParser_.good() && httpParser_.isMessageComplete()==false)
    {
        read(cc);
        if(this->bad())
            return content_;
        httpParser_.parse(buffer_.data(), lastBufferSize_);
    }

    return content_;
}


}



