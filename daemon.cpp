//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  daemon.cpp
//
//   Description:  进程监视与守护实现
//
//       Version:  1.0
//       Created:  2013年12月12日 17时06分37秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#ifndef WIN32
	#include<fcntl.h>
	#include<unistd.h>
	#include<sys/wait.h>
	#include<sys/stat.h>
	#include<sys/types.h>
#else
	#include<fcntl.h>
	#include<sys\types.h>
	#include<sys\stat.h>
	#include<io.h>
	#include<stdio.h>
#endif

#include<cstdlib>
#include<cstdio>
#include<string>
#include<thread>
#include<core/os.hpp>

#include"time.hpp"
#include"daemon.hpp"

namespace core
{

MainExitCode::Code_t MainExitCode::code_=MainExitCode::eCodeSuccess;
int MainExitCode::waitCount_;

static inline void daemonMsgWrite(const char* msg)
{
    std::string str=nowStringForestall();
    str += ": ";
    str += msg;
	#if !defined(WIN32)
    ::write(STDERR_FILENO, str.data(), str.size());
    ::fsync(STDERR_FILENO);
    #else
    std::cerr << str << std::endl;
	#endif
}

static inline std::string daemonLogName(const char* name)
{
    std::string file="logs";
    file += std::string("/")
        + name
        + "." + nowPathStringForestall()
        + ".pid"  + boost::lexical_cast<std::string>(::getpid())
        + ".dlog";
    return file;
}

void Daemon::start(const char* name, int& argc, const char** argv)
{
    GMacroUnUsedVar(name);
    GMacroUnUsedVar(argc);
    GMacroUnUsedVar(argv);

#if !defined(DEBUG) && defined(NDEBUG)

#ifdef WIN32

    bool noDaemon=false;
    if(std::strcmp(argv[argc-1], "--NoCoreDaemon")==0) //是子进程
        noDaemon=true;
    if(noDaemon==true)
    {
        argc -= 1;
        argv[argc]=nullptr;
        return;
    }

    //把标准输出重定向到文件
    for(int i=0; i<3; ++i)
        ::_close(i);
    int fd=::_open(daemonLogName(name).c_str(), _O_RDWR|_O_APPEND|_O_CREAT|_O_BINARY);
    ::_dup(fd);
    ::_dup(fd);

    for (;;)
    {
        core::Process worker(core::OS::getProgramPath().string());
        for (int i = 1; i < argc; ++i) //不复制程序名
            worker.arg(argv[i]);
        worker.arg("--NoCoreDaemon");

        worker.start();
        worker.wait();

        exitCodeDispatch(worker.exitCode());
    }

#else

    //把自己变成后台进程
    auto pid=::fork();
    switch(pid)
    {
        case 0:
            break;
        case -1:
            std::cerr << "Start Failed" << std::endl;
            std::exit(EXIT_FAILURE);
        default:
            std::exit(EXIT_SUCCESS);
    }

    //隔离终端
    ::setsid();

    //把标准输出重定向到文件
    for(int i=0; i<3; ++i)
        ::close(i);
    int fd=::open(daemonLogName(name).c_str(), O_RDWR|O_APPEND|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    ::dup(fd);
    ::dup(fd);

    //启动双进程，一个监视，一个工作
    //当子进程出错时，父进程再创建一个新的子进程干活
    for(;;)
    {
        daemonMsgWrite("perpare to create son work process\n");
        pid=::fork();
        switch(pid)
        {
            case 0: //子进程出去执行既定任务
                return;
            case -1:
                daemonMsgWrite("Start Failed\n");
                std::exit(EXIT_FAILURE);
            default: //父进程监护子进程
                watch(pid);
                ::fsync(fd);
        }
    }

#endif //WIN32
#endif //DEBUG && !NDEBUG
}

void Daemon::watch(int pid)
{
    GMacroUnUsedVar(pid);
#if !defined(DEBUG) && defined(NDEBUG) && !defined(WIN32)
    int status=0;
    ::waitpid(pid, &status, 0);

    if(WIFEXITED(status))
    {
        int exitStat=WEXITSTATUS(status);
        exitCodeDispatch(exitStat);
    }

    //子进程是非正常退出，再创建子进程
#endif
}

void Daemon::exitCodeDispatch(int exitCode)
{
    switch(exitCode)
    {
        case MainExitCode::eCodeFailure:
        {
            daemonMsgWrite("work process exit with failure, watch process exit\n");
            std::exit(EXIT_FAILURE);
        }
        case MainExitCode::eCodeSuccess:
        {
            daemonMsgWrite("work process exit normal, watch process exit\n");
            std::exit(EXIT_SUCCESS);
        }
        case MainExitCode::eCodeRestart:
        {
            daemonMsgWrite("work process restart\n");
            break;
        }
        case MainExitCode::eCodeWait:
        {
            std::string msg("work process wait resource:waitCount:");
            msg += std::to_string(MainExitCode::waitCountGet());
            msg += "\n";
            daemonMsgWrite(msg.c_str());

            std::this_thread::sleep_for(std::chrono::seconds(1));
            MainExitCode::waitCountShift();
            break;
        }
        default:
        {
            daemonMsgWrite("work process abort, rerun it\n");
            break;
        }
    }
}


}

