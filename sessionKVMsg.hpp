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

#include"time.hpp"
#include"codec.hpp"
#include"server.hpp"
#include"session.hpp"
#include"message.hpp"

namespace core
{

template<typename Obj, typename Msg>
class SessionKVMsgT: public SSessionT<TCPIOUnit>
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

    SessionKVMsgT(Stream&& stm)
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
            core::decode(readBuf_, msg);

            if(msg.empty())
                return ecSet(CoreError::ecMake(CoreError::eNetProtocolError));

            //验证消息编号
            typedef typename Msg::Key Key;
            auto& index=msg.get(Key::eMsgIndex);
            if(index.invalid()
                || index.tag()!=core::SimpleValue_t::eInt
                || static_cast<uint32_t>(index.intGet())!=remoteMsgIndex_++)
            {
                return ecSet(CoreError::ecMake(CoreError::eNetProtocolError));
            }

            objGet().dispatch(msg);
            if(bad())
                return;
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

    void packetRead()
    {
        uint32_t len=0;
        if(readBuf_.size()<sizeof(len))
            readBuf_.resize(sizeof(len));
        auto& stm=ioUnitGet();
        stm.read(readContext_, ecGet(), boost::asio::buffer(const_cast<char*>(readBuf_.data()), sizeof(len)));
        if(bad())
            return;

        core::decode(readBuf_, len);
        if(len>=eMaxBufSize)
            return ecSet(CoreError::ecMake(CoreError::eNetProtocolError));

        if(readBuf_.size()<len)
            readBuf_.resize(len);
        stm.read(readContext_, ecGet(), boost::asio::buffer(const_cast<char*>(readBuf_.data()), len));
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
                ioUnitGet().write(writeContext_, ecGet(), writeQueue_.front());
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

private:
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

}

