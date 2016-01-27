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
#include"server.hpp"
#include"session.hpp"
#include"message.hpp"

#if 0
namespace core
{

extern template class PacketIOUnitT<
    boost::asio::ip::tcp::socket,
    boost::asio::ip::tcp::acceptor,
    boost::asio::ip::tcp::endpoint,
    boost::asio::ip::tcp::resolver
    >;

typedef PacketIOUnitT<
    boost::asio::ip::tcp::socket,
    boost::asio::ip::tcp::acceptor,
    boost::asio::ip::tcp::endpoint,
    boost::asio::ip::tcp::resolver
    > TCPPacketIOUnit;

extern template class TableIOUnitT<
    boost::asio::ip::tcp::socket,
    boost::asio::ip::tcp::acceptor,
    boost::asio::ip::tcp::endpoint,
    boost::asio::ip::tcp::resolver
    >;

typedef TableIOUnitT<
    boost::asio::ip::tcp::socket,
    boost::asio::ip::tcp::acceptor,
    boost::asio::ip::tcp::endpoint,
    boost::asio::ip::tcp::resolver
    > TCPTableIOUnit;


extern template class SSessionT<TCPPacketIOUnit>;
extern template class SSessionT<TCPTableIOUnit>;
extern template class CSessionT<TCPPacketIOUnit>;
extern template class CSessionT<TCPTableIOUnit>;

//=============================================================
//common
namespace details
{
struct SSessionClose
{
    void operator()(const std::shared_ptr<SSession>& ptr)
    {
        ptr->streamClose();
    }
};

class SSessionReadTimer: public TimerWheelT<SSessionReadTimer, 60, SSession, SSessionClose>
{
    typedef TimerWheelT<SSessionReadTimer, 60, SSession, SSessionClose> BaseThis;

    struct SSUnit
    {
        SSUnit(const std::shared_ptr<TimerUnit>& p, unsigned where)
            :weak(p), last(where)
        {}

        SSUnit(SSUnit&& o)
            :weak(std::move(o.weak)), last(o.last)
        {}

        std::weak_ptr<TimerUnit> weak;
        size_t last;
    };

public:
    SSessionReadTimer()
        :BaseThis(1000)
    {}

    static void update(const std::shared_ptr<SSession>& p);
};

template<typename Object, bool isSS>
struct OptT;

//服务端会话
template<typename Object>
struct OptT<Object, true>
{
    static bool safeCheck(Object& obj, ProCmd_t cmd)
    {
        return obj.safeCheck(cmd);
    }

    static bool cmdCheck(ProCmd_t cmd)
    {
        return Object::Command::isCmd(cmd);
    }

    static void handShake(CoroutineContext& yield, Object& obj, SoftVersion& version)
    {
        auto ret=HandShake::service(yield, obj, Object::Service::traitGet(), Object::Command::versionCurrentGet());
        if(yield.bad())
            return;
        version=ret;
    }

    static void timerUpdate(const std::shared_ptr<SSession>& p)
    {
        SSessionReadTimer::update(p);
    }

    static const char* cmdString(ProCmd_t cmd)
    {
        return Object::Command::cmdString(cmd);
    }
};

//客户端会话
template<typename Object>
struct OptT<Object, false>
{
    static bool safeCheck(Object& , ProCmd_t )
    {
        return true;
    }

    static bool cmdCheck(ProCmd_t cmd)
    {
        return Object::Command::isResult(cmd);
    }

    static void handShake(CoroutineContext& yield, Object& obj, SoftVersion& version)
    {
        auto ret=HandShake::client(yield, obj, Object::Service::traitGet(), Object::Command::versionCurrentGet());
        if(yield.bad())
            return;
        version=ret;
    }

    static void timerUpdate(const std::shared_ptr<CSession>& )
    {}

    static const char* cmdString(ProCmd_t cmd)
    {
        return Object::Command::resultString(cmd);
    }

};

template<typename Object, typename Base>
class TableSessionMiddleT: public Base
{
    typedef Base BaseThis;

protected:
    Object& objGet()
    {
        return static_cast<Object&>(*this);
    }

    typedef std::function<void()> CallFun;
public:
    using BaseThis::BaseThis;

    void exitCallPush(CallFun&& callIn)
    {
        //需要串行处理，因为会话结束是异步事件
        objGet().strandGet().post(
            [this, handle=std::move(callIn)]()
            {
                if(!exitCalled_)
                {
                    exitCalls_.emplace_back(std::move(handle));
                    return;
                }

                ioserviceGet().post(std::move(handle));
            }
        );
    }

    IOService& ioserviceGet()
    {
        return IOService::cast(Base::ioUnitGet().streamGet().get_io_service());
    }

    void exitCall()
    {
        GMacroSessionLog(objGet(), SeverityLevel::info)
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

    void commandRead(CoroutineContext& yield)
    {
        auto& io=this->ioUnitGet();
        while(this->condRun() && !this->isReadShutDown())
        {
            easyTableReadAsync(yield, io.streamGet(), io);
            if(yield.bad())
                return;

            //针对客户端与服务端会话进行不能的操作
            typedef OptT<Object, std::is_base_of<SSession, Object>::value> Opt;

            auto& obj=this->objGet();
            if(obj.isExternSSession())
                Opt::timerUpdate(obj.shared_from_this());

            const auto& table=io.readTableGet();
            if(table.sizeGet()<=0 || table.sizeGet(0)<=0)
                return yield.ecSet(GMacroECMake(eNetProtocolError));

            const ProCmd_t* cmdVal=boost::get<ProCmd_t>(&table.valueGet(0, 0));
            if(cmdVal==nullptr || !Opt::cmdCheck(*cmdVal))
                return yield.ecSet(GMacroECMake(eNetProtocolError));

            GMacroSessionLog(objGet(), SeverityLevel::info)
                << "commandRead:" << Opt::cmdString(*cmdVal);

            const auto safe=Opt::safeCheck(obj, *cmdVal);
            if(!safe)
                return;

            obj.commandDispatch(yield, *cmdVal);
            if(yield.bad())
                return;
        }
    }

    void handShake(CoroutineContext& yield)
    {
        typedef OptT<Object, std::is_base_of<SSession, Object>::value> Opt;
        Opt::handShake(yield, objGet(), peerVersion_);
    }

    constexpr bool isExternSSession() const
    {
        return false;
    }

    const SoftVersion& peerVersionGet() const
    {
        return peerVersion_;
    }

protected:
    //会话准备退出时回调，也就是会话已经没有任务需要完成
    //回调调用时，会话仍然有效，但已经停止
    //回调返回后，会话随时可能失效
    bool exitCalled_=false;
    std::list<CallFun> exitCalls_;

    SoftVersion peerVersion_;
};

/**
* @brief 读写双线的中间层
*
* @tparam Object 最终类型
* @tparam Base 中间层基
*/
template<typename Object, typename Base>
class DuplexTableSessionBaseT: public TableSessionMiddleT<Object, Base>
{
    typedef TableSessionMiddleT<Object, Base> BaseThis;
    typedef std::function<void(CoroutineContext& , typename BaseThis::IOUnit& , ErrorCode& )>  WriteCall;
    struct WriteUnit
    {
        typedef enum
        {
            eTable,
            eBuffer,
        }Type_t;

        WriteUnit(const EasyTable& tab)
            : type(eTable)
            , table(tab)
        {}

        WriteUnit(EasyTable&& tab)
            : type(eTable)
            , table(std::move(tab))
        {}

        WriteUnit(const std::shared_ptr<ByteBuffer>& buf)
            : type(eBuffer)
            , buffer(buf)
        {}

        template<typename Call>
        WriteUnit(const EasyTable& tab, Call&& c)
            : type(eTable)
            , table(tab)
            , call(std::move(c))
        {}

        template<typename Call>
        WriteUnit(EasyTable&& tab, Call&& c)
            : type(eTable)
            , table(std::move(tab))
            , call(std::move(c))
        {}

        template<typename Call>
        WriteUnit(const std::shared_ptr<ByteBuffer>& buf, Call&& c)
            : type(eBuffer)
            , buffer(buf)
            , call(std::move(c))
        {}



        Type_t type;
        union
        {
            EasyTable table;
            std::shared_ptr<ByteBuffer> buffer;
        };

        WriteCall call;

        ~WriteUnit()
        {
            switch(type)
            {
                case eTable:
                    destruct(&table);
                    break;
                case eBuffer:
                    destruct(&buffer);
                    break;
            }
        }
    };

public:
    DuplexTableSessionBaseT()
        : strand_(this->ioserviceGet().castGet())
        , writeContext_(strand_)
    {}

    DuplexTableSessionBaseT(typename Base::IOUnit&& strm)
        : BaseThis(std::move(strm))
        , strand_(this->ioserviceGet().castGet())
        , writeContext_(strand_)
    {}

    void sessionStart()
    {
        namespace ba=boost::asio;
        auto self=this->shared_from_this();

        GMacroSessionLog(this->objGet(), SeverityLevel::info)
            << "sessionStart:" << Object::Service::serviceName();

        readContext_.spawn(this->ioserviceGet(),
            [self, this]()
            {
                readCoro();

                GMacroSessionLog(this->objGet(), SeverityLevel::info)
                    << "readCoroEnd:waitWriteCoro";

                //等待写协程
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

                this->exitCall();
            }
        );
    }

    IOService::Strand& strandGet()
    {
        return strand_;
    }

    void tableWrite(CoroutineContext&, EasyTable&& tbl)
    {
        tableWrite(std::move(tbl), WriteCall());
    }

    void tableWrite(EasyTable&& tbl)
    {
        tableWrite(std::move(tbl), WriteCall());
    }

    template<typename Call>
    void tableWrite(EasyTable&& tblIn, Call&& callIn)
    {
        if(this->isWriteShutDown())
            return;

        strandGet().post(
            [tbl=std::move(tblIn), call=std::move(callIn), this]()
            {
                tables_.emplace(std::move(tbl), std::move(call));
                if(!writeNeedRecall_)
                    return;

                writeNeedRecall_=false;
                writeContext_.resume();
            }
        );
    }

    void bufferWrite(const std::shared_ptr<ByteBuffer>& buf)
    {
        bufferWrite(buf, WriteCall());
    }

    template<typename Call>
    void bufferWrite(const std::shared_ptr<ByteBuffer>& buf, Call&& callIn)
    {
        if(this->isWriteShutDown())
            return;

        strandGet().post(
            [buf, call=std::move(callIn), this]()
            {
                tables_.emplace(std::move(buf), std::move(call));
                if(!writeNeedRecall_)
                    return;

                writeNeedRecall_=false;
                writeContext_.resume();
            }
        );
    }

    ~DuplexTableSessionBaseT()
    {
        assert(writeContext_.isStoped());
        assert(this->good() ? tables_.empty() : true );
    }

    void exitSet()
    {
        strandGet().post(
            [this]()
            {
                BaseThis::exitSet();
            }
        );
    }

    template<typename Call>
    void tagCallSet(ProUInt_t tag, Call&& call)
    {
        this->ioUnitGet().tagCallSet(tag, std::move(call));
    }

private:
    void writeCoro()
    {
        const auto& sc=scopedCall(
            [this]()
            {
                assert(this->good() ? tables_.empty() : true );
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

            while(tables_.empty() && this->condRun())
            {
                writeNeedRecall_=true;
                writeContext_.yield();
                if(writeContext_.bad())
                {
                    this->ecSet(writeContext_.ecReadGet());
                    return;
                }
            }

            while(!tables_.empty() && this->good())
            {
                unitWrite(tables_.front());
                if(writeContext_.bad())
                    return;
                tables_.pop();
            }
        }
    }

    void unitWrite(WriteUnit& unit)
    {
        assert(unit.type==WriteUnit::eTable || unit.type==WriteUnit::eBuffer);
        auto& io=this->objGet().ioUnitGet();
        switch(unit.type)
        {
            case WriteUnit::eTable:
            {
                io.writeTableGet()=unit.table;
                easyTableWriteAsync(writeContext_, io, io.streamGet());
                if(writeContext_.bad())
                    return this->ecSet(writeContext_.ecReadGet());
                break;
            }
            case WriteUnit::eBuffer:
            {
                auto& stream=io.streamGet();
                bufferWriteAsync(writeContext_, *unit.buffer, stream);
                if(writeContext_.bad())
                    return this->ecSet(writeContext_.ecReadGet());
                break;
            }
        }

        if(!unit.call)
            return;
        unit.call(writeContext_, io, this->ecGet());
    }

    void readCoro()
    {
        auto errCall=[this]()
        {
            this->ecSet(readContext_.ecReadGet());
            this->errorMessageLog(readContext_.ecReadGet());
            this->sessionShutDown();
        };

        auto& obj=this->objGet();
        typedef OptT<Object, std::is_base_of<SSession, Object>::value> Opt;
        if(obj.isExternSSession())
            Opt::timerUpdate(obj.shared_from_this());

        obj.handShake(readContext_);
        if(readContext_.bad())
        {
            errCall();
            return;
        }

        writeInit();
        writeInitResume();

        obj.commandRead(readContext_);
        if(readContext_.bad())
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

    /**
    * @brief 初始化写协程
    */
    void writeInit()
    {
        writeContext_.create(
            [this]()
            {
                this->writeCoro();
            }
        );
    }

    /**
    * @brief 一次性启动写协程
    *   @details 为了保证handShake在最初被同步地处理，
    *       写协程需要在handShake之前一直处理暂停状态
    *       本函数被用作在handShake完成后，被调用
    */
    void writeInitResume()
    {
        strandGet().post(
            [this]()
            {
                writeContext_.resume();
            }
        );

    }


private:
    //写表格相关支持
    bool writeNeedRecall_=false;
    IOService::Strand strand_;
    CoroutineContext writeContext_;
    std::queue<WriteUnit> tables_;

    //读命令
    bool readNeedRecall_=false;
    CoroutineContext readContext_;
};

/**
* @brief 单路表格会话中间层
*
* @tparam Object 最终类型
* @tparam Base 基
*/
template<typename Object, typename Base>
class TableSessionBaseT: public TableSessionMiddleT<Object, Base>
{
    typedef TableSessionMiddleT<Object, Base> BaseThis;
public:
    using BaseThis::BaseThis;

    void tableWrite(CoroutineContext& yield, EasyTable&& tbl)
    {
        auto& io=this->ioUnitGet();
        io.writeTableGet()=std::move(tbl);
        easyTableWriteAsync(yield, io, io.streamGet());
    }

    void sessionStart()
    {
        auto self=this->shared_from_this();

        context_.spawn(this->ioserviceGet(),
            [self, this]()
            {
                sessonCoro();
                this->exitCall();
            }
        );
    }

private:
    void sessonCoro()
    {
        auto& obj=this->objGet();

        obj.handShake(context_);
        if(context_.bad())
        {
            this->errorMessageLog(context_.ecReadGet());
            return;
        }

        obj.commandRead(context_);
        if(context_.bad())
        {
            this->errorMessageLog(context_.ecReadGet());
            return;
        }

    }

private:
    CoroutineContext context_;
};

}

//=============================================================
//Service
template<typename Object>
using TableServiceT=ServiceT<Object, TCPTableIOUnit>;

template<typename Object>
using PacketServiceT=ServiceT<Object, TCPPacketIOUnit>;

template<typename Object, size_t DictN>
class DuplexTableSSessionT: public details::DuplexTableSessionBaseT<Object, SSessionT<TCPTableIOUnit>>
{
    typedef details::DuplexTableSessionBaseT<Object, SSessionT<TCPTableIOUnit>> BaseThis;
public:
    using BaseThis::BaseThis;

    bool safeCheck(ProCmd_t)
    {
        return true;
    }

};

//=============================================================
//ClientSession
template<typename Object, typename IOUnitType=TCPTableIOUnit>
using TableCSessionT=details::TableSessionBaseT<Object, CSessionT<IOUnitType>>;

/**
* @brief 客户端命令分发与回调中间层
*
* @tparam Object 最终类型
*/
template<typename Object, size_t DictN, typename IOUnitType=TCPTableIOUnit>
class DuplexTableCSessionT: public details::DuplexTableSessionBaseT<Object, TableCSessionT<Object, IOUnitType>>
{
    typedef details::DuplexTableSessionBaseT<Object, TableCSessionT<Object, IOUnitType>> BaseThis;

protected:
    typedef typename BaseThis::CmdPtr CmdPtr;
    typedef std::array<CmdPtr, DictN> Dict;

    DuplexTableCSessionT()
    {
        //在退出之前需要安全地处理所有客户端命令对象
        this->exitCallPush(
            [this]()
            {
                cmdClosed_=true;
                for(auto& cmd: dict_)
                {
                    if(!cmd)
                        continue;
                    cmd->callReset();
                    cmd.reset();
                }
            }
        );
    }

public:
    void commandDispatch(CoroutineContext& yield, ProCmd_t cmdVal)
    {
        typedef typename Object::Command Command;

        if(!Command::isResult(cmdVal))
            return yield.ecSet(GMacroECMake(eNetProtocolError));

        const auto cmd=Command::resultCast(cmdVal);
        CmdPtr& ptr=cmdDictGet()[Command::resultHash(cmd)];
        if(!ptr || ptr->traitGet().replyGet()!=cmd)
            return yield.ecSet(GMacroECMake(eNetProtocolError));

        switch(cmd)
        {
            case Command::eResultNone:
            case Command::eResultCount:
            case Command::eResultHandShake:
                return yield.ecSet(GMacroECMake(eNetProtocolError));
            default:
            {
                ptr->result(this->ioUnitGet().readTableGet());

                if(ptr->finished())
                {
                    auto tmp(std::move(ptr));
                    return;
                }

                if(ptr->bad())
                {
                    typedef details::OptT<Object, std::is_base_of<SSession, Object>::value> Opt;
                    GMacroSessionLog(this->objGet(), SeverityLevel::info)
                        << "commandDispatch:"
                        << Opt::cmdString(ptr->traitGet().requestGet())
                        << ":EC:" << ptr->ecGet().message();

                    auto tmp(std::move(ptr));
                    return;
                }

                break;
            }
        }
    }

    void commandExecute(CoroutineContext& yield, const CmdPtr& cmd)
    {
        this->strandGet().post(
            [this, cmd]()
            {
                cmdRegister(cmd);
            }
        );
        cmd->request(yield);
    }

    void commandExecute(const CmdPtr& cmd)
    {
        this->strandGet().post(
            [this, cmd]()
            {
                cmdRegister(cmd);
            }
        );
        cmd->request();
    }

    void commandExecute(CoroutineContext& yield, const CmdPtr& cmd, ErrorCode& ec)
    {
        this->strandGet().post(
            [this, cmd]()
            {
                cmdRegister(cmd);
            }
        );
        cmd->request(yield, ec);
        if(ec.good())
            return;

        //出错也从字典里删除
        this->strandGet().post(
            [this, cmd]()
            {
                cmdRemove(cmd);
            }
        );
    }

    void commandExecute(const CmdPtr& cmd, ErrorCode& ec)
    {
        this->strandGet().post(
            [this, cmd]()
            {
                cmdRegister(cmd);
            }
        );
        cmd->request(ec);
        if(ec.good())
            return;

        //出错也从字典里删除
        this->strandGet().post(
            [this, cmd]()
            {
                cmdRemove(cmd);
            }
        );
    }

    template<typename Itr>
    void commandExecute(Itr b, Itr e)
    {
        for(; b!=e; ++b)
            commandExecute(*b);
    }

    void sessionShutDown()
    {
        if(sessionExitCommand_)
            return;

        sessionExitCommand_=true;
        typedef CCmdSessionExitT<Object> SessionExit;
        auto ptr=std::make_shared<SessionExit>(this->objGet());
        ptr->callReset(
            [this]()
            {
                this->BaseThis::sessionShutDown();
            }
        );
        commandExecute(ptr);
    }

    void sessionShutDownOnly()
    {
        BaseThis::sessionShutDown();
    }

protected:

    Dict& cmdDictGet()
    {
        return dict_;
    }

private:
    void cmdRegister(const CmdPtr& cmd)
    {
        //当前句柄已经关闭
        if(cmdClosed_)
            return;

        typedef typename Object::Command Command;
        const auto rlt=Command::resultCast(cmd->traitGet().replyGet());
        const auto idx=Command::resultHash(rlt);

        GMacroSessionLog(this->objGet(), SeverityLevel::info)
            << "cmdRegister:" <<  Command::resultString(rlt);

        assert(idx<dict_.size());
        dict_[idx]=std::move(cmd);
    }

    void cmdRemove(const CmdPtr& cmd)
    {
        typedef typename Object::Command Command;
        const auto rlt=Command::resultCast(cmd->traitGet().replyGet());
        const auto idx=Command::resultHash(rlt);

        assert(idx<dict_.size());

        auto tmp(std::move(dict_[idx]));
        GMacroUnUsedVar(tmp);
    }
private:
    bool cmdClosed_=false;
    bool sessionExitCommand_=false;
    Dict dict_;
};

template<typename Object>
using SingleTableCSessionT=TableCSessionT<Object, details::SingleIOUnit>;

template<typename Object, size_t DictN>
using SingleDuplexTableCSessionT=DuplexTableCSessionT<Object, DictN, details::SingleIOUnit>;




}
#endif

namespace core
{

class SessionKVMsg: public SSessionT<TCPIOUnit>
{
    typedef SSessionT<TCPIOUnit> BaseThis;
    enum
    {
        eMaxBufSize=16*1024*1024-64
    };

public:
    typedef std::function<void()> ExitCall;

    SessionKVMsg(Stream&& stm)
        : BaseThis(std::move(stm))
        , strand_(this->ioserviceGet().castGet())
        , writeContext_(strand_)
    {}

    void sessionStart();

    void write(const std::string& str);
    void write(const MessageBase& msg);

    void exitCallPush(ExitCall&& callIn);

    IOService& ioserviceGet();
private:
    void readCoro();
    void writeCoroWait();
    void messageRead();
    virtual void messageProcess(MessageBase& )=0;

    void writeInit();
    void writeCoro();

    auto& strandGet()
    {
        return strand_;
    }
    void exitCall();
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

};

}

