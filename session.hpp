//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  session.hpp
//
//   Description:  服务器框架会话接口
//
//       Version:  1.0
//       Created:  2013年04月07日 13时02分52秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================

#pragma once

#include"io.hpp"
#include"log.hpp"
#include"error.hpp"
#include"server.hpp"

#include<atomic>
#include<memory>
#include<boost/asio/read.hpp>
#include<boost/asio/write.hpp>
#include<boost/asio/ip/tcp.hpp>
#include<boost/asio/connect.hpp>
#include<boost/asio/io_service.hpp>

namespace core
{

#define GMacroSessionLog(CppSession, CppLevel) \
    BOOST_LOG_SEV((CppSession).sessionLogGet().logGet(), CppLevel) \
        << (CppSession).sessionMsgGet()

/**
* @brief 会话相关的常量
*/
class SessionCount
{
public:
    enum
    {
        eDefaultBufferSize=32*1024-16 ///< 会话连接缓存区大小
    };
};

typedef uint64_t SessionID_t;

class SSessionData
{
public:
    SSessionData()
    {
        byte_.fill(0u);
    }

    typedef enum : uint32_t
    {
        eNone=0,
        eReadTimer,

        //======================
        eDataCount,
    }DataType_t;

    typedef std::array<SSessionData, eDataCount-1> Array;

    typedef void (*Destructor)(void* ptr);

    template<typename Value>
    static void destruct(void* v)
    {
        delete static_cast<Value*>(v);
    }

    template<typename Value, typename... Args>
    void emplace(DataType_t dt, Args&&... args)
    {
        static_assert(sizeof(Value)<=sizeof(byte_), "sizeof error");
        type_=dt;
        desstructor_=&destruct<Value>;
        new (byte_.data()) Value(std::forward<Args&&>(args)...);
    }

    template<typename Value>
    Value& get()
    {
        static_assert(sizeof(Value)<=sizeof(byte_), "sizeof error");
        return *reinterpret_cast<Value*>(byte_.data());
    }

    template<typename Value>
    const Value& get() const
    {
        static_assert(sizeof(Value)<=sizeof(byte_), "sizeof error");
        return *reinterpret_cast<const Value*>(byte_.data());
    }

    DataType_t typeGet() const
    {
        return type_;
    }

    bool empty() const
    {
        return typeGet()==eNone;
    }

    void clear()
    {
        if(empty())
            return;
        if(desstructor_)
            desstructor_(static_cast<void*>(byte_.data()));
        type_=eNone;
        desstructor_=nullptr;
        byte_.fill(0);
    }

    ~SSessionData()
    {
        clear();
    }

private:
    DataType_t type_=eNone;
    Destructor desstructor_=nullptr;
    std::array<Byte, 32> byte_;
};

namespace details
{
    template<typename Base>
    class SessionBaseT: public Base
    {
        SessionBaseT(const SessionBaseT&)=delete;
        SessionBaseT& operator=(const SessionBaseT&)=delete;
    private:
        static SessionID_t sessionIDAlloc()
        {
            static std::atomic<SessionID_t> gs;
            return gs++;
        }

    public:
        SessionBaseT(const char* msgPrefix)
            : isExit_(false)
            , isReadShutDown_(false)
            , isWriteShutDown_(false)
            , sessionID_(sessionIDAlloc())
            , sessionMsg_(std::string(msgPrefix) + ':' + std::to_string(sessionID_) + ':')
        {}

        virtual void sessionStart() =0;

        virtual void sessionShutDown()
        {
            this->readShutDownSet();
            this->writeShutDownSet();
            this->exitSet();
        }

        virtual void streamClose() = 0;
        virtual bool isConnected() const = 0;

        Logger& sessionLogGet() const
        {
            return sessionLog_;
        }

        virtual ~SessionBaseT()=default;

        SessionID_t sessionIDGet() const
        {
            return sessionID_;
        }

        const std::string& sessionMsgGet() const
        {
            return sessionMsg_;
        }

        bool isExit() const
        {
            return isExit_;
        }

        virtual void exitSet()
        {
            isExit_=true;
            isReadShutDown_=true;
            isWriteShutDown_=true;
        }

        void readShutDownSet()
        {
            isReadShutDown_=true;
        }

        void writeShutDownSet()
        {
            isWriteShutDown_=true;
        }

        bool isReadShutDown() const
        {
            return isReadShutDown_;
        }

        bool isWriteShutDown() const
        {
            return isWriteShutDown_;
        }

        void errorMessageLog(const ErrorCode& ec)
        {
            GMacroSessionLog(*this, SeverityLevel::error)
                << "EC:" << ec.message();
        }

        bool condRun() const
        {
            return !isExit() && this->good();
        }

    protected:
        bool isExit_;
        bool isReadShutDown_;
        bool isWriteShutDown_;

    private:
        SessionID_t const sessionID_;
        std::string const sessionMsg_;
        mutable Logger sessionLog_;
    };
}

class SSession: public details::SessionBaseT<ErrorBaseT<std::enable_shared_from_this<SSession>>>
{
    typedef details::SessionBaseT<ErrorBaseT<std::enable_shared_from_this<SSession>>> BaseThis;
public:
    SSession();
    virtual void sessionShutDown();
    virtual ~SSession();

    SSessionData& dateGet(SSessionData::DataType_t dt)
    {
        return const_cast<SSessionData&>(static_cast<const SSession*>(this)->dateGet(dt));
    }

    const SSessionData& dateGet(SSessionData::DataType_t dt) const
    {
        assert(dt>SSessionData::eNone && dt<SSessionData::eDataCount);
        const auto& u=data_[dt-1];
        assert(u.typeGet()==SSessionData::eNone || u.typeGet()==dt);
        return u;
    }

private:
    SSessionData::Array data_;
};

class CCmdBase;
class CoroutineContext;

class CSession: public details::SessionBaseT<ErrorBaseT<std::enable_shared_from_this<CSession>>>
{
    typedef details::SessionBaseT<ErrorBaseT<std::enable_shared_from_this<CSession>>> BaseThis;
public:

    typedef std::shared_ptr<CCmdBase> CmdPtr;

    CSession();

    void hostSet(const HostPoint& hp)
    {
        host_=hp;
    }

    const HostPoint& hostGet() const
    {
        return host_;
    }

    typedef std::function<void (const ErrorCode&)> ConnectHandler;

    virtual void connect(CoroutineContext& yield)  =0;
    virtual void connect(ConnectHandler&& handle)  =0;

    typedef std::function<void()> CallFun;
    virtual void exitCallPush(CallFun&& callIn)    =0;

    void connect(CoroutineContext& yield, const HostPoint& host)
    {
        this->hostSet(host);
        this->connect(static_cast<CoroutineContext&>(yield));
    }

    virtual ~CSession();
protected:
    HostPoint host_;
};

typedef std::shared_ptr<CSession> CSessionSharedPtr;

template<typename IOUnitType, typename Base=SSession>
class SSessionT: public Base
{
public:
    typedef IOUnitType IOUnit;
    typedef typename IOUnit::StreamType Stream;
    typedef typename IOUnit::EndPointType EndPoint;
    typedef typename IOUnit::ResolverType Resolver;

    SSessionT(Stream&& strm)
        : ioUnit_(std::move(strm))
    {}

    IOUnit& ioUnitGet()
    {
        return ioUnit_;
    }

    const IOUnit& ioUnitGet() const
    {
        return ioUnit_;
    }

    boost::asio::io_service& ioServiceGet()
    {
        return ioUnitGet().streamGet().get_io_service();
    }

    void streamClose()
    {
        ioUnit_.streamClose();
    }

    bool isConnected() const
    {
        return ioUnitGet().streamGet().is_open();
    }

protected:
    IOUnit ioUnit_;
};

namespace details
{
template<typename IOUnit, typename Handle>
void streamConnectAsync(Handle&& handle , const HostPoint& host, IOUnit& ioUnit)
{
    typedef typename IOUnit::ResolverType Resolver;
    Resolver resolver(ioUnit.streamGet().get_io_service());
    boost::asio::async_connect(ioUnit.streamGet(),
        resolver.resolve({host.addressGet().toString(), host.portGet().toString()}),
        [handle](const boost::system::error_code& ec, typename Resolver::iterator itr)
        {
            if(ec)
                return handle(ec);
            if(typename Resolver::iterator()==itr)
                return handle(core::CoreError::ecMake(core::CoreError::eNetConnectError));
            handle(core::CoreError::ecMake(core::CoreError::eGood));
        }
    );
}

template<typename IOUnit>
void streamConnectAsync(CoroutineContext& coro, const HostPoint& host, IOUnit& ioUnit)
{
    coro.yield(
        [&coro, host, &ioUnit]()
        {
            streamConnectAsync(
                [&coro](const boost::system::error_code& ec)
                {
                    if(ec)
                        coro.ecSet(ec);
                    coro.resume();
                }
            , host , ioUnit
            );
        }
    );
}


}

template<typename IOUnitType, typename Base=CSession>
class CSessionT: public Base
{
public:
    typedef IOUnitType IOUnit;
    typedef typename IOUnit::StreamType Stream;
    typedef typename IOUnit::EndPointType EndPoint;
    typedef typename IOUnit::ResolverType Resolver;
    typedef std::function<void (const ErrorCode&)> ConnectHandler;

    IOUnit& ioUnitGet()
    {
        return ioUnit_;
    }

    const IOUnit& ioUnitGet() const
    {
        return ioUnit_;
    }

    virtual ~CSessionT()
    {}

    using Base::connect;

    void connect(CoroutineContext& yield)
    {
        GMacroSessionLog(*this, SeverityLevel::info)
            << "connect:" << this->host_;

        streamConnectAsync(yield, this->host_, ioUnitGet());
        if(yield.bad())
        {
            GMacroSessionLog(*this, SeverityLevel::info)
                << "connect:failed";
            this->ecSet(yield.ecGet());
        }
    }

    void connect(ConnectHandler&& handle)
    {
        GMacroSessionLog(*this, SeverityLevel::info)
            << "CS:connect:" << this->host_;

        streamConnectAsync(
            [=](const ErrorCode& ec)
            {
                if(ec.bad())
                {
                    GMacroSessionLog(*this, SeverityLevel::info)
                        << "connect:failed";
                    this->ecSet(ec);
                }

                handle(ec);
            }
            , this->host_, ioUnitGet()
        );
    }

    bool isConnected() const
    {
        return ioUnitGet().streamGet().is_open();
    }

    void streamClose()
    {
        ioUnit_.streamClose();
    }

    void streamReset()
    {
        ioUnit_.streamReset();
    }

private:
    IOUnit ioUnit_;
};

#define GMacroSessionFunLog(CppSession) \
    BOOST_LOG_SEV((CppSession).sessionLogGet().logGet(), SeverityLevel::eDebug) \
    << (CppSession).sessionMsgGet()\
    << __FILE__  << ':' \
    << __LINE__;

namespace details
{

struct IOGetDefault
{
    static IOService& get()
    {
        return IOServer::serviceFetchOne();
    }
};

class IOBaseDefault
{

};

template<typename AsyncStream, typename AsyncAcceptor, typename EndPoint,
    typename Resolver, typename Base, typename IOGet=IOGetDefault>
class IOUnitT: public Base
{
public:
    typedef AsyncStream StreamType;
    typedef AsyncAcceptor AcceptorType;
    typedef EndPoint EndPointType;
    typedef Resolver ResolverType;

    IOUnitT(AsyncStream&& strm)
        :stream_(std::move(strm))
    {}

    IOUnitT()
        :stream_(IOGet::get().castGet())
    {}

    AsyncStream& streamGet()
    {
        return stream_;
    }

    const AsyncStream& streamGet() const
    {
        return stream_;
    }

    void streamClose()
    {
        stream_.close();
    }

    const HostAddress& remoteHostGet() const
    {
        if(remote_.empty())
        {
            const auto& add=remoteAddressGet();
            const_cast<HostAddress&>(remote_)=add.to_string();
        }
        return remote_;
    }

    const boost::asio::ip::address remoteAddressGet() const
    {
        return streamGet().remote_endpoint().address();
    }

    void streamReset()
    {
        stream_=AsyncStream(IOGet::get().castGet());
    }

private:
    HostAddress remote_;
    AsyncStream stream_;
};

}

template<typename Base=core::NullClass>
using TCPIOUnitT=details::IOUnitT<
    boost::asio::ip::tcp::socket,
    boost::asio::ip::tcp::acceptor,
    boost::asio::ip::tcp::endpoint,
    boost::asio::ip::tcp::resolver,
    Base
>;

class TCPIOUnit: public TCPIOUnitT<>
{
    typedef TCPIOUnitT<> BaseThis;
public:
    using BaseThis::BaseThis;

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

    template<typename Handle, typename... Args>
    void write(Args&&... args, Handle&& handle)
    {
        std::array<boost::asio::const_buffer, sizeof...(Args)> bufs{boost::asio::buffer(std::forward<Args&&>(args))...};
        boost::asio::async_write(streamGet(), bufs, std::move(handle));
    }

    template<typename... Args>
    void read(CoroutineContext& cc, ErrorCode& ecRet, Args&&... args)
    {
        std::array<boost::asio::mutable_buffer, sizeof...(Args)> bufs{boost::asio::buffer(std::forward<Args&&>(args))...};
        cc.yield([this, &cc, &ecRet, bufs]()
            {
                boost::asio::async_read(streamGet(), bufs,
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

    template<typename Handle, typename... Args>
    void read(Args&&... args, Handle&& handle)
    {
        std::array<boost::asio::mutable_buffer, sizeof...(Args)> bufs{boost::asio::buffer(std::forward<Args&&>(args))...};
        boost::asio::async_read(streamGet(), bufs, std::move(handle));
    }

};

}

