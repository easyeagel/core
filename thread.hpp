//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  thread.hpp
//
//   Description:  线程相关
//
//       Version:  1.0
//       Created:  2015年01月16日 09时44分13秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include<queue>

#include<boost/thread/tss.hpp>
#include<boost/thread/once.hpp>
#include<boost/thread/mutex.hpp>
#include<boost/thread/locks.hpp>
#include<boost/thread/thread.hpp>
#include<boost/thread/shared_mutex.hpp>
#include<boost/thread/condition_variable.hpp>

#include"lock.hpp"
#include"error.hpp"

namespace core
{

template<typename Unit>
class ConcurrencyQueueT
{
    typedef boost::unique_lock<boost::mutex> Lock;
public:
    ConcurrencyQueueT()
        :blocked_(true)
    {}

    Unit popTimed(const boost::system_time& absTime)
    {
        Lock lock(mutex_, absTime);
        if(!lock.owns_lock())
            return Unit();

        Unit t=queue_.front();
        queue_.pop();
        return std::move(t);
    }

    Unit popTry()
    {
        Lock lock(mutex_, boost::try_to_lock);
        if(!lock.owns_lock())
            return Unit();

        Unit t=queue_.front();
        queue_.pop();
        return std::move(t);
    }

    Unit pop()
    {
        Lock lock(mutex_);
        while(blocked_ && queue_.empty())
            cond_.wait(lock);

        if(queue_.empty())
            return Unit();

        Unit t=queue_.front();
        queue_.pop();
        return std::move(t);
    }

    size_t npop(Unit* dest, size_t n)
    {
        assert(dest && n);
        Lock lock(mutex_);
        while(queue_.empty())
            cond_.wait(lock);

        size_t ret=0;
        while(!queue_.empty() && n--)
        {
            *dest++=std::move(queue_.front());
            queue_.pop();
            ++ret;
        }

        return ret;
    }

    void push(Unit&& u)
    {
        Lock lock(mutex_);
        queue_.push(u);
        cond_.notify_one();
    }

    void push(const Unit& u)
    {
        Lock lock(mutex_);
        queue_.push(u);
        cond_.notify_one();
    }

    void npush(const Unit* src, size_t n)
    {
        assert(src && n);
        Lock lock(mutex_);
        for(size_t i=0; i<n; ++i)
            queue_.push(*src++);
        cond_.notify_all();
    }

    size_t sizeGet() const
    {
        return queue_.size();
    }

    void blockedSet(bool v)
    {
        blocked_=v;
        if(blocked_==false)
            cond_.notify_all();
    }

    ~ConcurrencyQueueT()
    {
        assert(queue_.empty());
    }

private:
    bool blocked_;
    boost::mutex mutex_;
    std::queue<Unit> queue_;
    boost::condition_variable cond_;
};

/**
* @brief 一个构造，静态对象的线程安全构造方法
*
* @tparam Obj 需要构造的类型
*/
template<typename Obj>
class OnceConstructT
{
public:
    template<typename... Args>
    OnceConstructT(Args&&... args)
    {
        static boost::once_flag once=BOOST_ONCE_INIT;
        boost::call_once(once, [this, &args...]() {
                onceConstruct(std::forward<Args&&>(args)...);
            }
        );
        waitComplete();
    }

    Obj& get()
    {
        waitComplete();
        return *ptr_;
    }

    const Obj& get() const
    {
        waitComplete();
        return *ptr_;
    }

private:
    void waitComplete()
    {
        while(!ptr_)
            /* 空循环 */;
    }

    template<typename... Args>
    void onceConstruct(Args&&... args)
    {
        ptr_.reset(new Obj(std::forward<Args&&>(args)...));
    }

private:
    std::shared_ptr<Obj> ptr_;
};

/**
* @brief 单例模式，如果一个类是单例的，则应该继承自该模板
* @see ThreadInstanceT
*
* @tparam T 需要实现单例的类
* @tparam Base 可选的基，它可能是实现 @a T 必须条件
*/
template<typename T, typename Base=NullClass>
class SingleInstanceT: public Base
{
    //移除复制，单便对象不应该复制
    SingleInstanceT(const SingleInstanceT&)=delete;
    SingleInstanceT& operator=(const SingleInstanceT&)=delete;
protected:

    //SingleInstanceT()=default;

    template<typename... Args>
    SingleInstanceT(Args&&... args)
        :Base(std::forward<Args&&>(args)...)
    {}

public:
    static T& instance()
    {
        static OnceConstructT<T> gs;
        return gs.get();
    }
};

/**
* @brief 线程单例模式，如果一个类是线程单例的，则应该继承自该模板
* @details
*   @li 所谓线程单例就是每个线程有一个独立实例
*   @li 线程单例避免同步的一个方法，参考 @ref SingleInstanceT
*
* @see SingleInstanceT
*
* @tparam T 需要实现单例的类
* @tparam Base 可选的基，它可能是实现 @a T 必须条件
*/
template<typename T, typename Base=NullClass>
class ThreadInstanceT: public Base
{
    ThreadInstanceT(const ThreadInstanceT&)=delete;
    ThreadInstanceT& operator=(const ThreadInstanceT&)=delete;
protected:
    template<typename... Args>
    ThreadInstanceT(Args&&... args)
        :Base(std::forward<Args&&>(args)...)
    {}

    //ThreadInstanceT()=default;

public:
    static T& instance()
    {
        static boost::thread_specific_ptr<T> gsPtr;
        T* ptr=gsPtr.get();
        if(ptr==nullptr)
        {
            ptr=new T;
            gsPtr.reset(ptr);
        }

        assert(ptr && gsPtr.get());
        return *ptr;
    }
};

}


