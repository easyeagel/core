//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  server.hpp
//
//   Description:  服务器框架
//
//       Version:  1.0
//       Created:  2013年03月05日 17时15分56秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  magicred.net
//
//=====================================================================================

#pragma once

#include<functional>

#include<boost/optional.hpp>
#include<boost/asio/spawn.hpp>
#include<boost/asio/strand.hpp>
#include<boost/thread/thread.hpp>
#include<boost/asio/io_service.hpp>
#include<boost/system/error_code.hpp>
#include<boost/asio/steady_timer.hpp>
#include<boost/coroutine/asymmetric_coroutine.hpp>

#include"thread.hpp"

namespace core
{
class IOServer;
class IOService;
class MainServer;
class ThreadThis;

class IOService
    : private boost::asio::io_service
{
    friend class IOServer;

    typedef enum {eStatusUnStart, eStatusWorking, eStatusStopped} Status_t;

    typedef std::unique_ptr<boost::thread> ThreadPtr;
    typedef boost::asio::io_service BaseThis;
    typedef std::function<void()> SimpleCall;
public:

    typedef BaseThis::strand Strand;

    IOService();

    void run(SimpleCall&& call=SimpleCall());
    void stopSignal();
    void stop();

    void reset();

    bool stopped() const
    {
        return status_==eStatusStopped;
    }

    using BaseThis::post;
    using BaseThis::poll;

    boost::asio::io_service& castGet()
    {
        return *this;
    }

    static IOService& cast(boost::asio::io_service& o)
    {
        return static_cast<IOService&>(o);
    }

    void justRun(SimpleCall&& call=SimpleCall())
    {
        threadRun(std::move(call));
    }

    void join();

private:
    void threadRun(SimpleCall&& call);

private:
    Status_t status_=eStatusUnStart;
    ThreadPtr thread_;
    boost::optional<boost::asio::io_service::work> workGuard_;
};

/**
* @brief 主线程工作服务器
* @details
*   @li 本类是单例，并且主线程将工作在此
*/
class MainServer
    : public SingleInstanceT<MainServer>
{
    typedef boost::asio::io_service ASIOService;
    friend class OnceConstructT<MainServer>;

    MainServer();

public:
    typedef std::function<void()> CallFun;

    static void start();
    static void stop();

    template<typename Fun>
    static void post(Fun&& fun)
    {
        instance().ios_.post(std::move(fun));
    }

    static ASIOService& get()
    {
        return instance().ios_.castGet();
    }

    static IOService& serviceGet()
    {
        return instance().ios_;
    }

    static void exitCallPush(CallFun&& fun)
    {
        auto& obj=instance();
        Spinlock::ScopedLock lock(obj.mutex_);

        obj.exitCalls_.emplace_back(std::move(fun));
    }

private:
    bool stopped_;

    Spinlock mutex_;
    IOService ios_;
    std::vector<CallFun> exitCalls_;
};

/**
* @brief 提供每个CPU有 2 个线程，每线程为一个 @ref IOService 工作
* @details
*   @li 如果系统只有一个CPU，则只启动一个线程；如果有多个CPU，则每个CPU启动 2 个线程
*   @li 每个 @ref IOService 不可能有多个线程共享，请使用 boost::asio::ioservice::strand 进行同步
*
* @see IOService, ServicePool
*/
class IOServer
    : public SingleInstanceT<IOServer>
{
    enum {eThreadCountPerCPU=2};
    enum {eStopWaitSecond=64, eStopWaitTimes=64};

    IOServer(const IOServer&)=delete;
    IOServer& operator=(const IOServer&)=delete;
    typedef boost::asio::steady_timer SteadyTimer;

public:
    typedef std::function<void()> CallFun;
    typedef std::function<void(CallFun&& fun)> StopCallFun;

    IOServer();

    static void stop();
    static void join();

    /**
    * @brief 停机回调
    * @details
    *   @li 调用时服务器框架还没有停止
    *   @li 按照加入时相反的顺序调用
    *
    * @param fun 被调用的对象
    */
    static void stopCallPush(StopCallFun&& fun);

    /**
    * @brief 开机回调
    * @detalis
    *   @li 调用时，框架还没有起来，所以当前是一个线程
    *
    * @param fun 被调用的对象
    */
    static void startCallPush(CallFun&& fun);

    static IOService& serviceFetchOne();

    static IOService& serviceFetch(size_t idx)
    {
        auto& asio=instance().asio_;
        return asio[idx%asio.size()];
    }

    template<typename Handle>
    static void post(size_t idx, Handle&& handle)
    {
        serviceFetch(idx).post(std::move(handle));
    }

    /**
    * @brief 在线程池执行指定调用，调用不会立即执行
    *           它只是简单地加入任务队列，稍后将被执行
    *
    * @tparam Handle 调用类型
    * @param handle 调用对象
    */
    template<typename Handle>
    static void post(Handle&& handle)
    {
        serviceFetchOne().post(std::move(handle));
    }

    /**
    * @brief 在每一个线程里的每个线程执行指定调用
    *
    * @tparam Handle 调用类型
    * @param handle 调用对象，它将复制到每个线程，并并发执行
    */
    template<typename Handle>
    static void postForeach(Handle&& handle)
    {
        for(auto& u: instance().asio_)
            u.post(handle); //这里需要复制
    }

    /**
    * @brief 在线程池内循环执行指定调用，直到调用返回 @a true
    * @details
    *   @li 调用的签名为: bool call()
    *   @li 调用返回一个bool值，没有参数需要给出
    *   @li 当返回值为 true 时，循环将终止，否则调用对象将再次被调度并调用
    *
    * @tparam Handle 调用类型
    * @param handle 调用对象
    */
    template<typename Handle>
    static void postLoop(Handle&& handleIn)
    {
        post([=, handle=std::move(handleIn)]()
            {
                bool done=handle();
                if(done)
                    return;
                postLoop(std::move(handle));
            }
        );
    }

    /**
    * @brief 依次在每个线程里调用指定调用；可选地，在所有调用结束后，执行回调
    *
    * @tparam Handle 任务调用类型
    * @tparam Complete 结束调用类型
    * @param handle 任务调用对象
    * @param complete 结束调用对象
    */
    template<typename Handle, typename Complete=VoidCallBack>
    static void postForeachStrand(Handle&& handle, Complete&& complete=Complete())
    {
        instance().postNextIndex(0, std::move(handle), std::move(complete));
    }

    static unsigned ioServiceCountGet()
    {
        unsigned const ncpu=cpuCount();
        return eThreadCountPerCPU*ncpu;
    }

private:
    static unsigned cpuCount()
    {
        static unsigned ncpu=boost::thread::hardware_concurrency();
        return ncpu;
    }

    template<typename Handle, typename Complete>
    void postNextIndex(size_t idx, Handle&& handleIn, Complete&& completeIn)
    {
        asio_[idx].post([this, idx, handle=std::move(handleIn), complete=std::move(completeIn)]()
            {
                handle();
                const auto next=idx+1;
                if(next>=asio_.size())
                {
                    complete();
                    return;
                }

                postNextIndex(next, std::move(handle), std::move(complete));
            }
        );
    }

    static void stopNext();

    void stopWait();

    void start();

private:
    static bool started_;
    std::atomic<unsigned> count_; ///< 用于分配IOService的计数
    std::vector<IOService> asio_;

    size_t waitTimes_;
    SteadyTimer timer_;

    //统一的系统启动机制
    Spinlock mutex_;
    std::vector<CallFun> startCall_;
    std::vector<StopCallFun> stopCall_;
};

/**
* @brief 计算服务器，执行长时间计算任务；此类任务没有时间要求
* @todo 实现固定线程数的调度与执行
*/
class ComputeServer
    : public SingleInstanceT<ComputeServer>
{
    typedef std::unique_ptr<boost::thread> ThreadPtr;
    friend class OnceConstructT<ComputeServer>;
    ComputeServer();
public:
    typedef std::function<void()> Computor;

    template<typename Handle>
    static void post(Handle&& handle)
    {
        auto& inst=instance();
        assert(static_cast<bool>(inst.stopped_)==false);
        inst.queue_.push(Computor(std::move(handle)));
    }

    static void stop (std::function<void()>&& stopCall);

    static bool isStopped()
    {
        return instance().stopped_;
    }

    static void join();

private:
    void threadTask();
    void startImpl();

private:
    static bool started_;
    std::atomic<bool> stopped_;
    std::function<void()> stopCall_;
    std::vector<ThreadPtr> threads_;
    std::atomic<uint32_t> threadCount_;
    ConcurrencyQueueT<Computor> queue_;
};

/**
* @brief 当前线程对象
* @details
*   @li 获取当前线程的IOService对象，如果当前线程运行了某个IOService
* @note 这应该是总是总是成立的，当 @ref IOServer 运行之后；用户不应该自己创建线程
*/
class ThreadThis
    : private ThreadInstanceT<ThreadThis>
{
    friend class IOService;
    friend class MainServer;
    friend class IOServer;
    friend class ComputeServer;
public:
    typedef enum
    {
        eThreadUnkown,
        eThreadMain,
        eThreadIOServer,
        eThreadComputeServer,
    }ThreadEnum_t;

    static IOService& serviceGet()
    {
        return *instance().io_;
    }

    static ThreadEnum_t enumGet()
    {
        return instance().threadEnum_;
    }

    static bool isSelf(const IOService& s)
    {
        return std::addressof(s)==std::addressof(serviceGet());
    }

    static bool isSelf(const boost::asio::io_service& s)
    {
        return std::addressof(s)==std::addressof(serviceGet().castGet());
    }

    static bool isMainThread()
    {
        return enumGet()==eThreadMain;
    }

    static bool isIOThread()
    {
        return enumGet()==eThreadIOServer;
    }

    static bool isComputeThread()
    {
        return enumGet()==eThreadComputeServer;
    }

    template<typename Handle>
    static void post(Handle&& handle)
    {
        serviceGet().post(std::move(handle));
    }

private:
    static void serviceSet(IOService* io)
    {
        instance().io_=io;
    }

    static void enumSet(ThreadEnum_t te)
    {
        instance().threadEnum_=te;
    }

private:
    IOService* io_;
    ThreadEnum_t threadEnum_=eThreadUnkown;
};

class CoroutineContext: public ErrorBase
{
    CoroutineContext(const CoroutineContext& other)=delete;
    CoroutineContext& operator=(CoroutineContext&& other)=delete;
    CoroutineContext& operator=(const CoroutineContext& other)=delete;

public:
    typedef std::function<void()> CallFun;
    typedef std::function<void(CallFun&& call)> ResumeCall;
    typedef boost::coroutines::asymmetric_coroutine<void>::pull_type PullType;
    typedef boost::coroutines::asymmetric_coroutine<void>::push_type PushType;

private:
    typedef std::unique_ptr<PullType> PullTypePtr;

public:
    CoroutineContext()
        :resumeCall_([](CallFun&& call){ call(); })
    {}

    CoroutineContext(ResumeCall&& call)
        :resumeCall_(call)
    {}

    CoroutineContext(IOService::Strand& strand)
        :resumeCall_([&strand](CallFun&& call){ strand.post(call); })
    {}

    CoroutineContext(CoroutineContext&& other)
        : resumeCall_(std::move(other.resumeCall_))
    {}

    template<typename Handler>
    void spawn(IOService& ios, Handler&& handle)
    {
        spawn(ios.castGet(), std::move(handle));
    }

    template<typename Handler>
    void spawn(boost::asio::io_service& ios, Handler&& handle)
    {
        ios.post(
            [this, &ios, handle]()
            {
                spawnTask(ios, std::move(handle));
            }
        );
    }

    template<typename Handler>
    void start(Handler&& handle)
    {
        resumeCall_([=]()
            {
                start(std::move(handle), true);
            }
        );
    }

    template<typename Handler>
    void create(Handler&& handle)
    {
        resumeCall_([=]()
            {
                start(std::move(handle), false);
            }
        );
    }

    template<typename Handler>
    void yield(Handler&& handle)
    {
        yiledCall_=handle;
        yield();
    }

    void yield();
    void resume();

    void resume(const boost::system::error_code& ec)
    {
        ecSet(ec);
        resume();
    }

    void coroReset();

    bool isStoped() const
    {
        if(!pull_)
            return true;
        return !static_cast<bool>(*pull_);
    }

    bool isYielded() const
    {
        return yielded_;
    }

 private:
    void coroutineReset(PullType* ptr)
    {
        assert(isStoped() && ptr);
        pull_.reset(ptr);
    }

    void callerReset(PushType& caller)
    {
        assert(isStoped());
        push_=&caller;
    }

    template<typename Handler>
    void start(Handler&& handle, bool resumed)
    {
        auto ptr = new PullType(
                [this, handle](PushType& caller)
                {
                    callerReset(caller);
                    yield();
                    handle();
                }
            );
        coroutineReset(ptr);

        if(!resumed)
            return;
        resume();
    }

    template<typename Handler>
    void spawnTask(boost::asio::io_service& ios, Handler&& handleIn)
    {
        start([this, &ios, handle=std::move(handleIn)]()
            {
                handle();

                //释放当前协程对象
                ios.post(
                    std::bind(&CoroutineContext::coroReset
                        , static_cast<CoroutineContext*>(this)));
            }
        );
    }

 private:
    PushType* push_=nullptr;
    PullTypePtr pull_;

    bool yielded_=true;
    CallFun yiledCall_;

    ResumeCall resumeCall_;
};

}

