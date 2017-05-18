//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  service.hpp
//
//   Description:  服务基础结构
//
//       Version:  1.0
//       Created:  2014年07月16日 10时37分16秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include"io.hpp"
#include"log.hpp"
#include"error.hpp"
#include"server.hpp"
#include"daemon.hpp"
#include"sessionPool.hpp"

#include<boost/asio/socket_base.hpp>
#include<boost/asio/steady_timer.hpp>

namespace core
{

class Service
    :public core::ErrorBaseT<std::enable_shared_from_this<Service>>
{
protected:
    typedef boost::asio::steady_timer SteadyTimer;

    enum {eAcceptWaitTime=10, eAcceptTryCount=1024 };


public:
    virtual ~Service()
    {}

    typedef enum
    {
        eStatusUnStart,
        eStatusStarting,
        eStatusWorking,
        eStatusPausing,
        eStatusPaused,
        eStatusStopping,
        eStatusStopped
    }Status_t;

    Service();

    typedef std::function<void ()> CallFun;

    virtual void start(CallFun fun) =0;
    virtual void stop (CallFun fun) =0;
    virtual void run  (CallFun fun) =0;
    virtual void pause(CallFun fun) =0;
    virtual void close() = 0;

    virtual const char* serviceName() const
    {
        return "notService";
    }

    void funCall()
    {
        if(!handle_)
            return;
        decltype(handle_) tmp(std::move(handle_));
        tmp();
    }

    Status_t statusGet() const
    {
        return status_;
    }

    void statusSet(Status_t st)
    {
    #ifdef DEBUG
        struct StatTran
        {
            Status_t now;
            Status_t next[2];
        };

        static StatTran statTran[]=
        {
            {eStatusWorking, {eStatusPausing, eStatusStopping} },
            {eStatusPaused , {eStatusWorking, eStatusStopping} },
            {eStatusStopped, {eStatusUnStart, eStatusUnStart} },
        };

        for(auto val: statTran)
        {
            if(val.now==status_)
            {
                assert(st==val.next[0] || st==val.next[1]);
                goto tagEnd;
            }
        }

        assert(static_cast<int>(st)==status_+1);

        tagEnd:
    #endif
        status_=st;
    }

    Logger& serviceLogGet() const
    {
        return serviceLog_;
    }

protected:
    CallFun handle_;
    Status_t status_=eStatusUnStart;
    mutable Logger serviceLog_;
    SteadyTimer timer_;
};

class SSession;
class JsonNode;

#define GMacroServiceLog(CppService, CppLevel) \
    BOOST_LOG_SEV((CppService).serviceLogGet().logGet(), CppLevel) \
            << "SV:" \
            << (CppService).serviceName() << ':'


//=======================================================================
template<typename Object, typename IOUnit>
class ServiceT: public Service
{
    typedef boost::asio::yield_context CoroutineContext;
protected:
    typedef typename IOUnit::StreamType Stream;
    typedef typename IOUnit::AcceptorType Acceptor;
    typedef typename IOUnit::EndPointType EndPoint;
    typedef typename IOUnit::ResolverType Resolver;
    virtual ~ServiceT()
    {}

public:
    ServiceT(const HostPoint& host)
        : stream_(IOServer::serviceFetchOne().castGet())
        , resolver_(MainServer::get())
        , acceptor_(MainServer::get())
        , acceptPoint_(*resolver_.resolve({host.addressGet().toString(), host.portGet().toString()}))
        , strand_(MainServer::get())
    {}

    void start(CallFun fun=CallFun());
    void stop (CallFun fun=CallFun());
    void run  (CallFun fun=CallFun());
    void pause(CallFun fun=CallFun());

    void close() final;

private:
    void acceptOpen();
    void acceptClose();
    void acceptCall(CoroutineContext& yield);
    void acceptWait(CoroutineContext& yield);
    void acceptPause(CoroutineContext& yield);
    void acceptStop();

    void errorRestart();

    void connectionProcess();

    bool safeCheck();

    std::shared_ptr<SSession> sessionStart();


protected:
    Stream stream_;

private:
    Resolver resolver_;
    Acceptor acceptor_;
    EndPoint acceptPoint_;
    IOService::Strand strand_;
};

}

namespace core
{

template<typename Object, typename IOUnit>
void ServiceT<Object, IOUnit>::start(CallFun fun)
{
    GMacroServiceLog(static_cast<const Object&>(*this), SeverityLevel::info)
        << "ServiceStart:" << acceptPoint_;

    handle_=std::move(fun);

    namespace bio=boost::asio;

    statusSet(eStatusStarting);

    auto self(this->shared_from_this());
    boost::asio::spawn(strand_,
        [this, self](bio::yield_context yield)->void
        {
            acceptOpen();
            if(bad())
            {
                GMacroServiceLog(static_cast<const Object&>(*this), SeverityLevel::error)
                    << "EC:acceptOpen:" << ecReadGet().message();
                return errorRestart();
            }

            statusSet(eStatusWorking);
            funCall();
            for(;;)
            {
                //循环接受新连接
                acceptor_.async_accept(stream_, yield[ecGet()]);
                acceptCall(yield);
                assert(statusGet()==eStatusWorking || statusGet()==eStatusStopped);
                switch(statusGet())
                { //此时的状态只能是stopped 或 working
                    case eStatusWorking:
                        continue;
                    default:
                        return;
                }
            }
        }
    );
}

template<typename Object, typename IOUnit>
void ServiceT<Object, IOUnit>::acceptOpen()
{
    namespace bio=boost::asio;

    //打开并绑定端口
    acceptor_.open(acceptPoint_.protocol(), this->ecGet());
    if(this->bad())
        return;

    acceptor_.set_option(bio::socket_base::reuse_address(true));
    acceptor_.bind(acceptPoint_, this->ecGet());
    if(this->bad())
        return;

    acceptor_.listen(bio::socket_base::max_connections, this->ecGet());
    if(this->bad())
        return;
}

template<typename Object, typename IOUnit>
void ServiceT<Object, IOUnit>::errorRestart()
{
    std::cerr << "error: " << acceptPoint_ << ": " << this->ecGet().message() << std::endl;
    MainExitCode::codeSet(MainExitCode::eCodeRestart);
    return IOServer::stop();
}

template<typename Object, typename IOUnit>
void ServiceT<Object, IOUnit>::acceptClose()
{
    GMacroServiceLog(static_cast<const Object&>(*this), SeverityLevel::error)
        << "EC:acceptClose";

    stream_=Stream(IOServer::serviceFetchOne().castGet());
    acceptor_.close();
}

template<typename Object, typename IOUnit>
void ServiceT<Object, IOUnit>::acceptCall(CoroutineContext& yield)
{
    switch(statusGet())
    {
        case eStatusWorking:
        {
            if(bad())
            {
                //出现了真正的错误，等待一段时间再作尝试
                GMacroServiceLog(static_cast<const Object&>(*this), SeverityLevel::error)
                    << "EC:" << ecReadGet().message();

                //出现真正错误后，服务状态发生改变，切换到暂停状态
                statusSet(eStatusPausing);
                statusSet(eStatusPaused);
                acceptWait(yield);
                return;
            }

            connectionProcess();
            return;
        }
        case eStatusPausing:
        {
            if(good()) //在发起暂停之前已经有连接
                connectionProcess();
            acceptPause(yield);
            return;
        }
        case eStatusStopping:
        {
            if(good())
                connectionProcess();
            acceptStop();
            return;
        }
        case eStatusUnStart:
        case eStatusStarting:
        case eStatusPaused:
        case eStatusStopped:
        default:
        {
            GMacroServiceLog(static_cast<const Object&>(*this), SeverityLevel::fatal)
                << "EC:" << GMacroWhereMsg("Bug Found.");

            //不可能的状态，如果出错，则一个Bug
            GMacroAbort("Never Here, Must Have a Bug.");
            return;
        }
    }
}

template<typename Object, typename IOUnit>
void ServiceT<Object, IOUnit>::acceptWait(CoroutineContext& yield)
{
    //每秒测试一次，直到成功
    //或部测试次数大于 eAcceptWaitTime
    for(int i=0; i<eAcceptWaitTime; ++i)
    {
        acceptClose();
        timer_.expires_from_now(std::chrono::seconds(5));
        timer_.async_wait(yield[ecGet()]);
        ecClear();
        acceptOpen();
        if(bad())
        {
            GMacroServiceLog(static_cast<const Object&>(*this), SeverityLevel::error)
                << "EC:acceptOpen:" << ecReadGet().message();
            continue;
        }

        statusSet(eStatusWorking);
        return;
    }

    //尝试多次，重启当前进程
    errorRestart();
}

template<typename Object, typename IOUnit>
void ServiceT<Object, IOUnit>::acceptPause(CoroutineContext& yield)
{
    acceptClose();
    statusSet(eStatusPaused);
    funCall();

    timer_.expires_from_now(SteadyTimer::duration::max());
    timer_.async_wait(yield[ecGet()]);

    ecClear();
    acceptOpen();
    if(good())
    {
        statusSet(eStatusWorking);
        funCall();
        return;
    }
   
    GMacroServiceLog(static_cast<const Object&>(*this), SeverityLevel::error)
        << "EC:acceptOpen:" << ecReadGet().message();
    acceptWait(yield);
    funCall();
}

template<typename Object, typename IOUnit>
std::shared_ptr<SSession> ServiceT<Object, IOUnit>::sessionStart()
{
    auto ptr=std::make_shared<typename Object::SessionType>(std::move(stream_));

    auto sz=SessionManager::push(ptr);
    if(sz==SessionManager::eNotSize)
    {
        ptr.reset();
        return ptr;
    }

    //在指定Service里开始会话
    ptr->ioserviceGet().post(
        [ptr]()
        {
            ptr->sessionStart();
        }
    );

    return ptr;
}

template<typename Object, typename IOUnit>
void ServiceT<Object, IOUnit>::acceptStop()
{
    acceptClose();
    statusSet(eStatusStopped);
    funCall();
}

template<typename Object, typename IOUnit>
void ServiceT<Object, IOUnit>::connectionProcess()
{
    core::ErrorCode ec;
    const auto local=stream_.local_endpoint(ec);
    if(ec.bad())
    {
        GMacroServiceLog(static_cast<const Object&>(*this), SeverityLevel::info)
            << "EC: " << ec.message();
        stream_=Stream(IOServer::serviceFetchOne().castGet());
        return;
    }

    const auto remote=stream_.remote_endpoint(ec);
    if(ec.bad())
    {
        GMacroServiceLog(static_cast<const Object&>(*this), SeverityLevel::info)
            << "EC: " << ec.message();
        stream_=Stream(IOServer::serviceFetchOne().castGet());
        return;
    }

    if(!safeCheck())
    {
        stream_=Stream(IOServer::serviceFetchOne().castGet());
        return;
    }

    auto ptr=this->sessionStart();

    //为下个连接选择新IOService，如此重新分配线程或CPU
    stream_=Stream(IOServer::serviceFetchOne().castGet());

    GMacroServiceLog(static_cast<const Object&>(*this), SeverityLevel::info)
        << ( ptr ? ptr->sessionMsgGet() : "SessionCount OverRun, Refused: ")
        << local
        << " <- "
        << remote;
}

template<typename Object, typename IOUnit>
bool ServiceT<Object, IOUnit>::safeCheck()
{
    //这里好像永远不出现，但是现实中出现了一个异常，似乎由这个发出的?
    const auto& remote=stream_.remote_endpoint(ecGet());
    if(bad())
    {
        GMacroServiceLog(static_cast<const Object&>(*this), SeverityLevel::info)
            << "EC: " << ecReadGet().message();
        return false;
    }

    const auto& radr=remote.address();
    if(radr.is_v4())
    {
        //const auto ok=SafeIPv4::check(static_cast<IPv4Adress_t>(radr.to_v4().to_ulong()), ecGet());
        //if(!ok)
        //{
             //GMacroServiceLog(static_cast<const Object&>(*this), SeverityLevel::warning)
                //<< "EC:" << ecReadGet().message()
                //<< ":"   << radr.to_string();
            //return false;
        //}
    }

    return true;
}

template<typename Object, typename IOUnit>
void ServiceT<Object, IOUnit>::run(CallFun fun)
{
    handle_=std::move(fun);

    strand_.post(
        [this]()
        {
            timer_.cancel();
        }
    );
}

template<typename Object, typename IOUnit>
void ServiceT<Object, IOUnit>::pause(CallFun fun)
{
    handle_=std::move(fun);

    strand_.post(
        [this]()
        {
            statusSet(eStatusPausing);
            acceptor_.cancel();
        }
    );
}

template<typename Object, typename IOUnit>
void ServiceT<Object, IOUnit>::stop(CallFun fun)
{
    //说明监听网络出错，在启动的过程中发起停机
    //此时监听事实上没有启动，所以直接回调
    if(status_==eStatusStarting)
        return fun();

    handle_=std::move(fun);
    strand_.post(
        [this]()
        {
            statusSet(eStatusStopping);
            acceptor_.cancel();
        }
    );
}

template<typename Object, typename IOUnit>
void ServiceT<Object, IOUnit>::close()
{
    acceptor_.close();
}


}

