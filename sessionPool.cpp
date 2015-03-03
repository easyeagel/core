//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  sessionPool.cpp
//
//   Description:  会话管理
//
//       Version:  1.0
//       Created:  2013年12月31日 15时37分48秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#include<sys/time.h>
#include<sys/resource.h>

#include"session.hpp"
#include"sessionPool.hpp"

namespace core
{

SessionManager::SessionManager()
    :sessionCountLimit_(eDefaultCountLimit)
{
    struct ::rlimit rl={0, 0};
    auto ret=::getrlimit(RLIMIT_NOFILE, &rl);
    assert(ret==0);
    if(ret!=0)
        return;
    assert(rl.rlim_cur>=1024);
    sessionCountLimit_=rl.rlim_cur-64;
}

void SessionManager::allClose()
{
    MutexScopedLock lock(instance().mutex_);
    for(auto& ses: instance().sets_)
        ses->streamClose();
}



}


