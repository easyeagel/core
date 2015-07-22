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
#include<core/httpSession.hpp>

namespace core
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
            auto s = const_cast<std::shared_ptr<core::SSession>&>(self);
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
        this->read(coroutine_);
        if(this->bad())
            return;

#ifndef NDEBUG
        std::cout << std::string(this->buffer_.data(), this->lastBufferSize_) << std::endl;
#endif

        this->httpParser_.parse(this->buffer_.data(), this->lastBufferSize_);

        if(this->httpParser_.bad())
            return;

        if(!this->httpParser_.isMessageComplete())
            continue;

        if(this->dispatch_)
        {
            auto respones=this->dispatch_->bodyCompleteCall(this->ecGet(), this->httpParser_);
            if(this->bad())
                return;
            this->write(coroutine_, *respones);
        }

        if(this->httpParser_.isKeep() && this->dispatch_->isKeep())
            continue;

        return;
    }
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

