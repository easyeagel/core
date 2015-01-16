//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  server.cpp
//
//   Description:  easyDB 服务器框架
//
//       Version:  1.0
//       Created:  2013年03月05日 17时30分41秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#include"server.hpp"

#include<boost/algorithm/string/predicate.hpp>

namespace core
{
void CoroutineContext::yield()
{
    assert(static_cast<bool>(push_));
    (*push_)();
}

void CoroutineContext::resume()
{
    resumeCall_([this]()
        {
            assert(yielded_==true);
            yielded_=false;
            assert(static_cast<bool>(pull_) && static_cast<bool>(*pull_));
            (*pull_)();
            assert(yielded_==false);
            yielded_=true;

            //下面代码不在协程里运行
            //并且在协程一次运行完成之后运行
            if(yiledCall_)
            {
                //解决循环引用问题，使用临时变量
                CallFun tmp(std::move(yiledCall_));
                tmp();
            }
        }
    );

}

void CoroutineContext::coroReset()
{
    assert(static_cast<bool>(pull_));

    PullTypePtr tmp;
    pull_.swap(tmp);

    while(*tmp)
        ; //忙等待

    assert(!static_cast<bool>(*tmp));
}


//======================================================================
MainServer::MainServer()
    :stopped_(false)
{}

void MainServer::start()
{
    auto& inst=instance();
    inst.ios_.justRun(
        []()
        {
            ThreadThis::enumSet(ThreadThis::eThreadMain);
        }
    );

    //调用进程退出回调，要求按照加入相反的顺序调用
    auto& calls=inst.exitCalls_;
    while(!calls.empty())
    {
        auto& call=calls.back();
        call();
        calls.pop_back();
    }

    IOServer::join();
    ComputeServer::join();

    inst.stopped_=true;
}

void MainServer::stop()
{
    instance().ios_.stopSignal();
    instance().ios_.stop();
}




//======================================================================
IOService::IOService()
    :workGuard_(boost::in_place(boost::ref(castGet())))
{}

void IOService::reset()
{
    BaseThis::reset();
    workGuard_=boost::in_place(boost::ref(castGet()));
}

void IOService::run(SimpleCall&& callIn)
{
    assert(!thread_);
    thread_.reset(new boost::thread(
            [this, call=std::move(callIn)]() mutable
            {
                threadRun(std::move(call));
            }
        )
    );
}

void IOService::threadRun(SimpleCall&& call)
{
    ThreadThis::serviceSet(this);
    status_=eStatusWorking;
    if(call)
        call();
    BaseThis::run();
    status_=eStatusStopped;
}

void IOService::stopSignal()
{
    workGuard_=boost::none;
}

void IOService::stop()
{
    BaseThis::stop();
}

void IOService::join()
{
    if(!thread_)
        return;
    thread_->join();
}



//======================================================================
bool IOServer::started_=false;

IOServer::IOServer()
    :count_(0), asio_(ioServiceCountGet()), waitTimes_(0), timer_(MainServer::get())
{
    start();
}

void IOServer::start()
{
    started_=true;

    for(const auto& call: startCall_)
        call();

    for(size_t i=0; i<asio_.size(); ++i)
    {
        asio_[i].run(
            []()
            {
                ThreadThis::enumSet(ThreadThis::eThreadIOServer);
            }
        );
    }
}

void IOServer::join()
{
    if(started_==false)
        return;

    auto& self=instance();
    for(auto& asio: self.asio_)
        asio.join();
}

void IOServer::stop()
{
    if(started_==false)
    {
        ComputeServer::stop([]()
            {
                MainServer::stop();
            }
        );
        return;
    }

    MainServer::post(
        []()
        {
            auto& self=instance();

            if(!self.stopCall_.empty())
                return stopNext();

            for(auto& itr: self.asio_)
                itr.stopSignal();

            self.stopWait();
        }
    );
}

void IOServer::stopWait()
{
    const auto& wait=std::chrono::milliseconds(std::chrono::seconds(eStopWaitSecond))/eStopWaitTimes;
    timer_.expires_from_now(wait);
    timer_.async_wait(
        [this](const boost::system::error_code& )
        {
            const auto computorStopCall=[]()
            {
                MainServer::stop();
            };

            //等一段时间，如果这段时间里还没有自动结束，强制结束
            if(++waitTimes_>eStopWaitTimes)
            {
                for(auto& itr: asio_)
                    itr.stop();
                ComputeServer::stop(std::move(computorStopCall));
                return;
            }

            for(auto& itr: asio_)
            {
                if(!itr.stopped())
                    return stopWait();
            }

            //直到这里则说明都结束了
            ComputeServer::stop(std::move(computorStopCall));
        }
    );


}

void IOServer::stopNext()
{
    auto& self=instance();

    auto tmp(std::move(self.stopCall_.back()));
    self.stopCall_.pop_back();

    tmp(
        []()
        {
            //允许同步调用
            //如果同步调用不把其post到队列里，则这将是递归调用
            //递归调用可能是危险的，如果有太多回调需要调用的话
            MainServer::post(
                []()
                {
                    stop();
                }
            );
        }
    );

}

IOService& IOServer::serviceFetchOne()
{
    auto& self=instance();
    auto const nios=self.asio_.size();
    const size_t idx=(self.count_++%(nios-1));
    return self.asio_[idx];
}

void IOServer::stopCallPush(StopCallFun&& fun)
{
    //许多对象可能调用这个方法，同时这些对象调用这个方法可能在多个线程内
    auto& self=instance();
    decltype(self.mutex_)::ScopedLock lock(self.mutex_);
    self.stopCall_.emplace_back(std::move(fun));
}

void IOServer::startCallPush(CallFun&& fun)
{
    auto& self=instance();
    decltype(self.mutex_)::ScopedLock lock(self.mutex_);
    self.startCall_.emplace_back(std::move(fun));
}





//======================================================================
bool ComputeServer::started_=false;

void ComputeServer::startImpl()
{
    started_=true;

    unsigned ncpu=boost::thread::hardware_concurrency();
    threads_.resize(ncpu);
    for(auto& thdp: threads_)
        thdp.reset(new boost::thread(std::bind(&ComputeServer::threadTask, this)));
}

ComputeServer::ComputeServer()
    :stopped_(false), threadCount_(0)
{
    startImpl();
}

void ComputeServer::threadTask()
{
    ThreadThis::enumSet(ThreadThis::eThreadComputeServer);
    ++threadCount_;
    size_t emptyCount=0;
    for(;;)
    {
        auto cpt=queue_.pop();
        if(cpt)
        {
            emptyCount=0;
            cpt();
            continue;
        }

        if(++emptyCount<1024)
            continue;

        stopped_=true;
        if(--threadCount_>0)
            return;

        if(!stopCall_)
            return;

        MainServer::post(std::move(stopCall_));
        return;
    }
}

void ComputeServer::join()
{
    if(started_==false)
        return;

    auto& inst=instance();
    for(auto& thdPtr: inst.threads_)
        thdPtr->join();

    //可能存在没有完成的任务，最后在主线程内完成
    //加入ComputeServer内的任务不应该进行IO操作
    while(auto cpt=inst.queue_.pop())
        cpt();
}

void ComputeServer::stop(std::function<void()>&& stopCall)
{
    if(started_==false)
    {
        MainServer::post(std::move(stopCall));
        return;
    }

    auto& inst=instance();
    inst.stopCall_=std::move(stopCall);
    inst.queue_.blockedSet(false);
}

}

