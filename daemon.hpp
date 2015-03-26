//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  daemon.hpp
//
//   Description:  进程监视与守护
//
//       Version:  1.0
//       Created:  2013年12月12日 17时06分14秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================

#pragma once

#include<cstdlib>

namespace core
{

class MainExitCode
{
public:
    typedef enum
    {
        eCodeSuccess=EXIT_SUCCESS,
        eCodeFailure=EXIT_FAILURE,
        eCodeRestart,
        eCodeWait
    }Code_t;

    enum { eWaitCountLimit=1024 };

    static void codeSet(Code_t cd)
    {
        code_=cd;
    }

    static Code_t codeGet()
    {
        return code_;
    }

    static void waitCountShift()
    {
        waitCount_ += 1;
    }

    static int waitCountGet()
    {
        return waitCount_;
    }

    static Code_t waitCodeIf()
    {
        if(waitCount_<=eWaitCountLimit)
            return eCodeWait;
        return eCodeFailure;
    }

private:
    static Code_t code_;
    static int waitCount_;
};

class Daemon
{
public:
    /**
    * @brief
    * @details 启动一个子进程作为服务进程，同时监视这个子进程资源使用
    *   如果子进程资源使用状态满足一定条件，可能需要让子进程退出
    */
    static void start(const char* name);



private:
    static void watch(int pid);
};

}


