//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  httpSession.hpp
//
//   Description:  http 会话
//
//       Version:  1.0
//       Created:  2015年03月03日 15时18分51秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include<core/session.hpp>

#include"http.hpp"

namespace ezweb
{

namespace details
{
    class HttpIOUnit
    {

    };
}

class HttpIOUnit: public core::TCPIOUnit<details::HttpIOUnit>
{
    typedef core::TCPIOUnit<details::HttpIOUnit> BaseThis;
public:
    GMacroBaseThis(HttpIOUnit);
};

template<typename IOUnit, typename Base>
class HttpSessionBase: public Base
{
    typedef Base BaseThis;
public:
    const HttpParser& parserGet() const
    {
        return httpParser_;
    }

protected:
    GMacroBaseThis(HttpSessionBase);

    void write(core::CoroutineContext& cc, const HttpMessage& hm)
    {
        cc.yield([this, &cc, &hm]()
            {
                std::array<boost::asio::const_buffer, 2> bufs{boost::asio::buffer(hm.headGet()), boost::asio::buffer(hm.bodyGet())};
                boost::asio::async_write(this->ioUnitGet().streamGet(), bufs,
                    [this, &cc](const boost::system::error_code& ec, size_t nb)
                    {
                        if(ec)
                            this->ecSet(ec);
                        lastBufferSize_=nb;
                        cc.resume();
                    }
                );
            }
        );
    }

    void read(core::CoroutineContext& cc)
    {
        constexpr size_t totalSize=8*1024;
        if(buffer_.size()<totalSize)
            buffer_.resize(totalSize);

        lastBufferSize_=0;
        cc.yield([this, &cc]() mutable
            {
                this->ioUnitGet().streamGet().async_read_some(boost::asio::buffer(buffer_),
                    [this, &cc](const boost::system::error_code& ec, size_t nb)
                    {
                        if(ec)
                            this->ecSet(ec);
                        lastBufferSize_=nb;
                        cc.resume();
                    }
                );
            }
        );
    }

protected:
    HttpParser httpParser_;

    std::vector<char> buffer_;
    size_t lastBufferSize_=0;
};

class HttpSSession: public HttpSessionBase<HttpIOUnit, core::SSessionT<HttpIOUnit>>
{
    typedef HttpSessionBase<HttpIOUnit, core::SSessionT<HttpIOUnit>> BaseThis;
    typedef HttpIOUnit::StreamType AsyncStream;
public:
    HttpSSession(AsyncStream&& hp);

    core::IOService& ioserviceGet()
    {
        return core::IOService::cast(BaseThis::ioUnitGet().streamGet().get_io_service());
    }

    void sessionStart() final;
    void sessionShutDown() final;
private:
    void loop();

    void httpGet();

private:
    bool close_=false;
    core::CoroutineContext coroutine_;
};

class HttpCSession: public HttpSessionBase<HttpIOUnit, core::CSessionT<HttpIOUnit>>
{
    typedef HttpSessionBase<HttpIOUnit, core::CSessionT<HttpIOUnit>> BaseThis;
    typedef HttpIOUnit::StreamType AsyncStream;
public:
    HttpCSession();

    core::IOService& ioserviceGet()
    {
        return core::IOService::cast(BaseThis::ioUnitGet().streamGet().get_io_service());
    }

    std::string& urlGet(core::CoroutineContext& cc, const std::string& url);

private:
    void sessionStart(){}
    void exitCallPush(CallFun&& ){}

    int contentCopy(const char* b, size_t n)
    {
        content_.append(b, n);
        return 0;
    }

private:
    std::string content_;
    HttpRequest httpRequest_;
};

}

