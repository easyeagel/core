//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  lock.hpp
//
//   Description:  各种锁
//
//       Version:  1.0
//       Created:  2013年04月02日 13时31分45秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#pragma once

#include<cassert>
#include<atomic>

namespace core
{

namespace details
{

template<typename Mutex>
class ScopedLockT
{
    bool locked_;
    Mutex& lock_;
public:
    ScopedLockT(Mutex& l)
        :locked_(false), lock_(l)
    {
        lock();
    }

    void lock()
    {
        assert(locked_==false);
        lock_.lock();
        locked_=true;
    }

    void unlock()
    {
        assert(locked_);
        lock_.unlock();
        locked_=false;
    }

    ~ScopedLockT()
    {
        if(!locked_)
            return;
        unlock();
    }
};

}

class Spinlock
{
    friend class details::ScopedLockT<Spinlock>;
    typedef enum {eLocked, eUnlocked} Status_t;

    std::atomic<Status_t> state_;

public:
    typedef details::ScopedLockT<Spinlock> ScopedLock;

    Spinlock()
        : state_(eUnlocked)
    {}

    void lock()
    {
        while (state_.exchange(eLocked, std::memory_order_acquire) == eLocked)
        { /*  busy-wait */ }
    }

    void unlock()
    {
        state_.store(eUnlocked, std::memory_order_release);
    }

};

class Nulllock
{
    friend class details::ScopedLockT<Nulllock>;

public:
    void lock()
    {}

    void unlock()
    {}

    typedef class details::ScopedLockT<Nulllock> ScopedLock;
};

}


