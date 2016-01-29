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
#include<core/http/http.hpp>

namespace core
{

class HttpIOUnit: public core::TCPIOUnit
{
    typedef core::TCPIOUnit BaseThis;
public:
    GMacroBaseThis(HttpIOUnit)

    template<typename... Args>
    void write(CoroutineContext& cc, ErrorCode& ecRet, Args&&... args)
    {
        std::array<boost::asio::const_buffer, sizeof...(Args)> bufs{boost::asio::buffer(std::forward<Args&&>(args))...};
        cc.yield([this, &cc, &ecRet, bufs]()
            {
                boost::asio::async_write(streamGet(), bufs,
                    [this, &cc, &ecRet](const boost::system::error_code& ec, size_t )
                    {
                        if(ec)
                            ecRet=ec;
                        cc.resume();
                    }
                );
            }
        );
    }
};

template<typename IOUnit, typename Base>
class HttpSessionBaseT: public Base
{
public:
    const HttpParser& parserGet() const
    {
        return httpParser_;
    }

    struct Command
    {
        static int versionCurrentGet()
        {
            return 1;
        }
    };

protected:
    template<typename... Args>
    HttpSessionBaseT(Args&&... args)
        :Base(std::forward<Args&&>(args)...)
    {
        this->httpParser_.onHeaderComplete([this]()
            {
                return headComplete();
            }
        );

        this->httpParser_.onBody([this](const char* b, size_t nb)
            {
                assert(dispatch_);
                if(!dispatch_)
                    return 2;

                dispatch_->bodyCall(this->ecGet(), httpParser_, b, nb);
                if(this->bad())
                    return 2;
                return 0;
            }
        );
    }

    void write(CoroutineContext& cc, const HttpMessage& hm)
    {
        auto& io=this->ioUnitGet();
        io.write(cc, this->ecGet(), hm.headGet());
        if(this->bad())
            return;
        hm.bodyWrite(cc, this->ecGet(), io);
    }

    void read(CoroutineContext& cc)
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

    int headComplete()
    {
        const auto m=httpParser_.methodGet();
        switch(m)
        {
            case HTTP_GET:
            case HTTP_POST:
                return headMethod(HttpDispatchDict::instance().get(m));
            default:
                return 2;
        }
    }
private:
    int headMethod(HttpDispatch& hg)
    {
        GMacroSessionLog(*this, SeverityLevel::info)
            << "httpMethod:" << this->httpParser_.hostGet() << this->httpParser_.pathGet();

        dispatch_=hg.create(this->httpParser_);
        if(!dispatch_)
            return 1;

        dispatch_->headCompleteCall(this->ecGet(), httpParser_);
        if(this->bad())
            return 2;

        return 0;
    }

protected:
    HttpParser httpParser_;
    HttpDispatch::DispatcherSPtr dispatch_;

    std::vector<char> buffer_;
    size_t lastBufferSize_=0;
};

class HttpSSession: public HttpSessionBaseT<HttpIOUnit, core::SSessionT<HttpIOUnit>>
{
    typedef HttpSessionBaseT<HttpIOUnit, core::SSessionT<HttpIOUnit>> BaseThis;
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

private:
    bool close_=false;
    core::CoroutineContext coroutine_;
};

class HttpCSession: public HttpSessionBaseT<HttpIOUnit, core::CSessionT<HttpIOUnit>>
{
    typedef HttpSessionBaseT<HttpIOUnit, core::CSessionT<HttpIOUnit>> BaseThis;
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

