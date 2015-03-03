//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  sessionPool.hpp
//
//   Description:  会话池，用于管理服务端会话
//
//       Version:  1.0
//       Created:  2013年12月31日 15时18分58秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#ifndef MR_SESSIONPOOL_HPP
#define MR_SESSIONPOOL_HPP

#include<memory>
#include<unordered_set>

#include"thread.hpp"

namespace core
{

class SSession;

///@todo 处理为线程本地对象，如此避免锁
class SessionManager: public SingleInstanceT<SessionManager>
{
    enum {eDefaultCountLimit=1024-64};

    typedef boost::mutex Mutex;
    typedef boost::unique_lock<Mutex> MutexScopedLock;

    typedef std::shared_ptr<SSession> Pointer;
    friend class OnceConstructT<SessionManager>;

    SessionManager();

public:
    enum : size_t { eNotSize=~0u };

    static size_t push(const Pointer& ptr)
    {
        return instance().pushImpl(ptr);
    }

    static size_t free(const Pointer& ptr)
    {
        return instance().freeImpl(ptr);
    }

    static void allClose();

private:
    size_t pushImpl(const Pointer& ptr)
    {
        MutexScopedLock lock(mutex_);
        if(sets_.size()>=sessionCountLimit_)
            return eNotSize;
        sets_.insert(ptr);
        return sets_.size();
    }

    size_t freeImpl(const Pointer& ptr)
    {
        MutexScopedLock lock(mutex_);
        sets_.erase(ptr);
        return sets_.size();
    }

private:
    Mutex mutex_;
    size_t sessionCountLimit_=eDefaultCountLimit;
    std::unordered_set<Pointer> sets_;
};



}




#endif //MR_SESSIONPOOL_HPP

