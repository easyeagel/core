//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  sessionTcp.hpp
//
//   Description:  TCP 服务器框架
//
//       Version:  1.0
//       Created:  2013年06月19日 12时58分51秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  magicred.net
//
//=====================================================================================


#pragma once

#include<deque>
#include<memory>
#include<functional>
#include<boost/asio/ip/tcp.hpp>

#include<core/time.hpp>
#include<core/value.hpp>
#include<core/codec.hpp>
#include<core/server.hpp>
#include<core/session.hpp>
#include<core/message.hpp>
#include<core/deviceCode.hpp>

namespace core
{
template<typename Obj, typename Msg>
class SSessionKVMsgT: public SSessionT<TCPIOUnit>
{
    typedef SSessionT<TCPIOUnit> BaseThis;
    enum
    {
        eMaxBufSize=16*1024*1024-64
    };

    Obj& objGet()
    {
        return static_cast<Obj&>(*this);
    }

    const Obj& objGet() const
    {
        return static_cast<Obj&>(*this);
    }

protected:
    void handShake(core::CoroutineContext& ctx)
    {
        typedef typename Msg::Cmd Cmd;
        typedef typename Msg::Key Key;

        Msg msg;
        msg.cmdSet(Cmd::eHandShake);
        msg.set(Key::eMsgIndex, msgIndex_++);
        msg.set(Key::eSessionID, sessionIDGet());

        ioUnitGet().write(ctx, ecGet(), msgToString(msg));
    }

public:
    typedef std::function<void()> ExitCall;

    SSessionKVMsgT(Stream&& stm)
        : BaseThis(std::move(stm))
        , strand_(this->ioserviceGet().castGet())
        , writeContext_(strand_)
        , msgIndex_(msgIndexInitGet())
        , remoteMsgIndex_(msgIndex_)
    {}

    void sessionStart()
    {
        namespace ba=boost::asio;
        auto self=this->shared_from_this();

        GMacroSessionLog(*this, SeverityLevel::info)
            << "sessionStart";

        readContext_.spawn(this->ioserviceGet(),
            [self, this]()
            {
                readCoro();

                GMacroSessionLog(*this, SeverityLevel::info)
                    << "readCoroEnd:waitWriteCoro";

                //等待写协程退出
                //把自己加入 strand 内，反复测试
                while(!writeContext_.isStoped())
                {
                    readContext_.yield(
                        [this]()
                        {
                            writeCoroWait();
                        }
                    );
                }

                //写协程已经退出，退出本会话
                exitCall();
            }
        );
    }

    void write(Msg& msg)
    {
        typedef typename Msg::Key Key;

#ifndef NDEBUG
        GMacroSessionLog(*this, SeverityLevel::info)
            << "WriteMsg:" << Msg::toString(msg.cmdGet());
#endif

        msg.set(Key::eMsgIndex, msgIndex_++);
        msg.set(Key::eSessionID, sessionIDGet());
        write(msgToString(msg));
    }

    static std::string msgToString(const Msg& msg)
    {
        auto str=core::encode(msg);
        std::string tmp=core::encode(static_cast<uint32_t>(str.size()));
        tmp += str;
        return std::move(tmp);
    }

    void exitCallPush(ExitCall&& callIn)
    {
        //需要串行处理，因为会话结束是异步事件
        strandGet().post(
            [this, handle=std::move(callIn)]()
            {
                if(!exitCalled_)
                {
                    exitCalls_.emplace_back(std::move(handle));
                    return;
                }

                //在本函数退出之后执行
                ioserviceGet().post(std::move(handle));
            }
        );
    }

    IOService& ioserviceGet()
    {
        return IOService::cast(BaseThis::ioUnitGet().streamGet().get_io_service());
    }

protected:
    void messageRead()
    {
        for(;;)
        {
            packetRead();
            if(bad())
                return;

            Msg msg;
            messageCheck(msg);
            if(bad())
                return;

            objGet().dispatch(msg);
            if(bad())
                return;
        }
    }

    void messageCheck(Msg& msg)
    {
        core::decode(readBuf_, msg);

        if(msg.empty())
            return ecSet(CoreError::ecMake(CoreError::eNetProtocolError, u8"空消息"));

        //验证消息编号
        typedef typename Msg::Key Key;
        auto& index=msg.get(Key::eMsgIndex);
        if(index.invalid()
            || index.tag()!=core::SimpleValue_t::eInt
            || static_cast<uint32_t>(index.intGet())!=remoteMsgIndex_++)
        {
            return ecSet(CoreError::ecMake(CoreError::eNetProtocolError, u8"消息编号不正确"));
        }
    }

    void write(const std::string& str)
    {
        strandGet().post(
            [str, this]() mutable
            {
                writeQueue_.push(std::move(str));
                if(!writeNeedRecall_)
                    return;
                writeNeedRecall_=false;
                writeContext_.resume();
            }
        );
    }

    void packetRead()
    {
        uint32_t len=0;
        if(readBuf_.size()<sizeof(len))
            readBuf_.resize(sizeof(len));
        auto& stm=ioUnitGet();
        stm.timerRead(readContext_, ecGet(), 20, boost::asio::buffer(const_cast<char*>(readBuf_.data()), sizeof(len)));
        if(bad())
            return;

        core::decode(readBuf_, len);
        if(len>=eMaxBufSize)
            return ecSet(CoreError::ecMake(CoreError::eNetProtocolError, u8"消息过大"));

        if(readBuf_.size()<len)
            readBuf_.resize(len);
        stm.timerRead(readContext_, ecGet(), 20, boost::asio::buffer(const_cast<char*>(readBuf_.data()), len));
    }

private:
    void readCoro()
    {
        auto errCall=[this]()
        {
            errorMessageLog(ecReadGet());
            sessionShutDown();
        };

        objGet().handShake(readContext_);
        if(bad())
            return errCall();

        writeInit();

        messageRead();
        if(bad())
            return errCall();
    }

    void writeCoroWait()
    {
        strandGet().post(
            [this]()
            {
                //写协程已经完成；或者还没有来得及设置退出状态，等待设置
                if(writeContext_.isStoped() || this->condRun())
                    return readContext_.resume();

                if(writeNeedRecall_)
                {
                    writeNeedRecall_=false;
                    writeContext_.resume();
                }

                //等写协程把所有表写完
                readNeedRecall_=true;
            }
        );
    }

    void writeInit()
    {
        writeContext_.start(
            [this]()
            {
                this->writeCoro();
            }
        );
    }

    void writeCoro()
    {
        const auto& sc=scopedCall(
            [this]()
            {
                assert(good() ? writeQueue_.empty() : true );
                writeShutDownSet();
                if(readNeedRecall_)
                {
                    readNeedRecall_=false;
                    readContext_.resume();
                }
            }
        );

        while(condRun())
        {
            if(writeQueue_.empty())
            {
                writeNeedRecall_=true;
                writeContext_.yield();
                if(writeContext_.bad())
                    return ecSet(writeContext_.ecReadGet());
            } else {
                auto& msg=writeQueue_.front();
#ifndef NDEBUG
                GMacroSessionLog(*this, SeverityLevel::info)
                    << "msgSize:" << msg.size() << " msgHead: " << StringHex::encode(msg.substr(0, 4));
#endif
                ioUnitGet().write(writeContext_, ecGet(), msg);
                if(bad())
                    return;
                writeQueue_.pop();
            }
        }
    }

    auto& strandGet()
    {
        return strand_;
    }

    void exitCall()
    {
        GMacroSessionLog(*this, SeverityLevel::info)
            << "exitCall";

        if(exitCalls_.empty())
            return;

        auto self=this->shared_from_this();
        ioserviceGet().post(
            [self, this]()
            {
                for(auto& fun: exitCalls_)
                {
                    auto tmp(std::move(fun));
                    tmp();
                }

                exitCalls_.clear();
                exitCalled_=true;
            }
        );
    }

    static uint32_t msgIndexInitGet()
    {
        static uint32_t gs=static_cast<uint32_t>(std::time(nullptr));
        return gs++;
    }

protected:
    //写组件
    bool writeNeedRecall_=false;
    IOService::Strand strand_;
    CoroutineContext writeContext_;
    std::queue<std::string> writeQueue_;

    //读组件
    bool readNeedRecall_=false;
    std::string readBuf_;
    CoroutineContext readContext_;

    //会话退出
    bool exitCalled_=false;
    std::list<ExitCall> exitCalls_;

    //消息编号
    std::atomic<uint32_t> msgIndex_;
    uint32_t remoteMsgIndex_;
};

template<typename Obj, typename Msg>
class CSessionKVMsgT: public CSessionT<TCPIOUnit>
{
    typedef CSessionT<TCPIOUnit> BaseThis;
    enum
    {
        eMaxBufSize=16*1024*1024-64
    };

    Obj& objGet()
    {
        return static_cast<Obj&>(*this);
    }

    const Obj& objGet() const
    {
        return static_cast<Obj&>(*this);
    }

protected:
    void handShake(core::CoroutineContext& )
    {
        packetRead();
        if(bad())
            return;

        Msg msg;
        core::decode(readBuf_, msg);
        if(msg.empty())
            return ecSet(CoreError::ecMake(CoreError::eNetProtocolError, u8"空消息"));

        typedef typename Msg::Key Key;

        auto& index=msg.get(Key::eMsgIndex);
        if(index.invalid() || (index.tag()!=core::SimpleValue_t::eInt))
            return ecSet(CoreError::ecMake(CoreError::eNetProtocolError, u8"消息编号不一致"));

        auto& sid=msg.get(Key::eSessionID);
        if(sid.invalid() || (sid.tag()!=core::SimpleValue_t::eInt64))
            return ecSet(CoreError::ecMake(CoreError::eNetProtocolError, u8"会话ID不一致"));

        toServiceMessageIndex_=index.intGet();
        fromServiceMessageIndex_=index.intGet();
        serviceSessionID_=sid.template get<core::SimpleValue_t::eInt64>();
    }

public:
    typedef std::function<void()> ExitCall;

    CSessionKVMsgT()
        : strand_(this->ioserviceGet().castGet())
        , writeContext_(strand_)
        , toServiceMessageIndex_(0)
        , fromServiceMessageIndex_(toServiceMessageIndex_)
    {}

    void sessionStart()
    {
        namespace ba=boost::asio;
        auto self=this->shared_from_this();

        GMacroSessionLog(*this, SeverityLevel::info)
            << "sessionStart";

        readContext_.spawn(this->ioserviceGet(),
            [self, this]()
            {
                connect(readContext_);
                if(this->bad())
                    return exitCall();

                readCoro();

                GMacroSessionLog(*this, SeverityLevel::info)
                    << "readCoroEnd:waitWriteCoro";

                //等待写协程退出
                //把自己加入 strand 内，反复测试
                while(!writeContext_.isStoped())
                {
                    readContext_.yield(
                        [this]()
                        {
                            writeCoroWait();
                        }
                    );
                }

                //写协程已经退出，退出本会话
                exitCall();
            }
        );
    }

    void write(Msg& msg)
    {
        typedef typename Msg::Key Key;

        msg.set(Key::eMsgIndex, toServiceMessageIndex_++);
        msg.set(Key::eSessionID, serviceSessionID_);

        auto& dc=core::DeviceCode::instance();
        msg.set(Key::eDeviceCode, dc.codeGet());

        write(msgToString(msg));
    }

    static std::string msgToString(const Msg& msg)
    {
        auto str=core::encode(msg);
        std::string tmp=core::encode(static_cast<uint32_t>(str.size()));
        tmp += str;
        return std::move(tmp);
    }

    void exitCallPush(ExitCall&& callIn)
    {
        //需要串行处理，因为会话结束是异步事件
        strandGet().post(
            [this, handle=std::move(callIn)]()
            {
                if(!exitCalled_)
                {
                    exitCalls_.emplace_back(std::move(handle));
                    return;
                }

                //在本函数退出之后执行
                ioserviceGet().post(std::move(handle));
            }
        );
    }

    IOService& ioserviceGet()
    {
        return IOService::cast(BaseThis::ioUnitGet().streamGet().get_io_service());
    }

    template<typename Call>
    void afterHandShakeCallSet(Call&& call)
    {
        afterHandShake_=std::move(call);
    }

protected:
    void messageRead()
    {
        for(;;)
        {
            packetRead();
            if(bad())
                return;

            Msg msg;
            messageCheck(msg);
            if(bad())
                return;

            objGet().dispatch(msg);
            if(bad())
                return;
        }
    }

    void messageCheck(Msg& msg)
    {
        core::decode(readBuf_, msg);

        if(msg.empty())
            return ecSet(CoreError::ecMake(CoreError::eNetProtocolError, u8"空消息"));

        //验证消息编号
        typedef typename Msg::Key Key;
        auto& index=msg.get(Key::eMsgIndex);
        if(index.invalid()
            || index.tag()!=core::SimpleValue_t::eInt
            || static_cast<uint32_t>(index.intGet())!=++fromServiceMessageIndex_)
        {
            return ecSet(CoreError::ecMake(CoreError::eNetProtocolError, u8"消息编号不一致"));
        }
    }

    void write(const std::string& str)
    {
        if(this->isWriteShutDown())
            return;

        strandGet().post(
            [str, this]() mutable
            {
                writeQueue_.push(std::move(str));
                if(!writeNeedRecall_)
                    return;
                writeNeedRecall_=false;
                writeContext_.resume();
            }
        );
    }

    void packetRead()
    {
        uint32_t len=0;
        if(readBuf_.size()<sizeof(len))
            readBuf_.resize(sizeof(len));
        auto& stm=ioUnitGet();
        stm.timerRead(readContext_, ecGet(), 20, boost::asio::buffer(const_cast<char*>(readBuf_.data()), sizeof(len)));
        if(bad())
            return;

        core::decode(readBuf_, len);
        if(len>=eMaxBufSize)
            return ecSet(CoreError::ecMake(CoreError::eNetProtocolError, u8"消息过大"));

        if(readBuf_.size()<len)
            readBuf_.resize(len);
        stm.timerRead(readContext_, ecGet(), 20, boost::asio::buffer(const_cast<char*>(readBuf_.data()), len));
    }

private:
    void readCoro()
    {
        auto errCall=[this]()
        {
            errorMessageLog(ecReadGet());
            sessionShutDown();
        };

        objGet().handShake(readContext_);
        if(bad())
            return errCall();

        if(afterHandShake_)
            afterHandShake_();

        writeInit();

        messageRead();
        if(bad())
            return errCall();
    }

    void writeCoroWait()
    {
        strandGet().post(
            [this]()
            {
                //写协程已经完成；或者还没有来得及设置退出状态，等待设置
                if(writeContext_.isStoped() || this->condRun())
                    return readContext_.resume();

                if(writeNeedRecall_)
                {
                    writeNeedRecall_=false;
                    writeContext_.resume();
                }

                //等写协程把所有表写完
                readNeedRecall_=true;
            }
        );
    }

    void writeInit()
    {
        writeContext_.start(
            [this]()
            {
                this->writeCoro();
            }
        );
    }

    void writeCoro()
    {
        const auto& sc=scopedCall(
            [this]()
            {
                assert(this->good() ? writeQueue_.empty() : true );
                this->writeShutDownSet();
                if(this->readNeedRecall_)
                {
                    this->readNeedRecall_=false;
                    this->readContext_.resume();
                }
            }
        );
        GMacroUnUsedVar(sc);

        while(this->condRun())
        {
            assert(writeNeedRecall_==false);

            while(writeQueue_.empty() && this->condRun())
            {
                writeNeedRecall_=true;
                writeContext_.yield();
                if(writeContext_.bad())
                {
                    this->ecSet(writeContext_.ecReadGet());
                    return;
                }
            }

            while(!writeQueue_.empty() && this->good())
            {
                auto& msg=writeQueue_.front();
                ioUnitGet().write(writeContext_, ecGet(), msg);
                if(this->bad())
                    return;
                writeQueue_.pop();
            }
        }
    }

    auto& strandGet()
    {
        return strand_;
    }

    void exitCall()
    {
        GMacroSessionLog(*this, SeverityLevel::info)
            << "exitCall";

        if(exitCalls_.empty())
            return;

        auto self=this->shared_from_this();
        ioserviceGet().post(
            [self, this]()
            {
                for(auto& fun: exitCalls_)
                {
                    auto tmp(std::move(fun));
                    tmp();
                }

                exitCalls_.clear();
                exitCalled_=true;
            }
        );
    }

    static uint32_t msgIndexInitGet()
    {
        static uint32_t gs=static_cast<uint32_t>(std::time(nullptr));
        return gs++;
    }

protected:
    //写组件
    bool writeNeedRecall_=false;
    IOService::Strand strand_;
    CoroutineContext writeContext_;
    std::queue<std::string> writeQueue_;

    //读组件
    bool readNeedRecall_=false;
    std::string readBuf_;
    CoroutineContext readContext_;
    std::function<void()> afterHandShake_;

    //会话退出
    bool exitCalled_=false;
    std::list<ExitCall> exitCalls_;

    //消息编号
    std::atomic<uint32_t> toServiceMessageIndex_;
    uint32_t fromServiceMessageIndex_;
    SessionID_t serviceSessionID_=0;
};

}

