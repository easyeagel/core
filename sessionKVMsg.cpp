//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  tcpServer.cpp
//
//   Description:  TCP Server 实例
//
//       Version:  1.0
//       Created:  2013年06月19日 13时04分16秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#include"codec.hpp"
#include"sessionKVMsg.hpp"

namespace core
{

void SessionKVMsg::sessionStart()
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

void SessionKVMsg::writeCoroWait()
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

void SessionKVMsg::write(const std::string& str)
{
    strandGet().post(
        [str, this]()
        {
            writeQueue_.push(str);
            if(!writeNeedRecall_)
                return;
            writeNeedRecall_=false;
            writeContext_.resume();
        }
    );
}

void SessionKVMsg::write(const MessageBase& msg)
{
    write(core::encode(msg));
}

void SessionKVMsg::readCoro()
{
    auto errCall=[this]()
    {
        errorMessageLog(ecReadGet());
        sessionShutDown();
    };

    writeInit();

    messageRead();
    if(bad())
        return errCall();
}

void SessionKVMsg::writeInit()
{
    writeContext_.start(
        [this]()
        {
            this->writeCoro();
        }
    );
}

void SessionKVMsg::writeCoro()
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

void SessionKVMsg::messageRead()
{
    uint32_t len=0;
    auto& stm=ioUnitGet();
    stm.read(readContext_, ecGet(), boost::asio::buffer(&len, sizeof(len)));
    if(bad())
        return;

    if(len>=eMaxBufSize)
        return ecSet(CoreError::ecMake(CoreError::eNetProtocolError));

    readBuf_.resize(len);
    stm.read(readContext_, ecGet(), boost::asio::buffer(const_cast<char*>(readBuf_.data()), len));
    if(bad())
        return;

    MessageBase msg;
    core::decode(readBuf_, msg);

    if(msg.empty())
        return ecSet(CoreError::ecMake(CoreError::eNetProtocolError));

    messageProcess(msg);
}

IOService& SessionKVMsg::ioserviceGet()
{
    return IOService::cast(BaseThis::ioUnitGet().streamGet().get_io_service());
}

void SessionKVMsg::exitCall()
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

void SessionKVMsg::exitCallPush(ExitCall&& callIn)
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



}

