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



#include<fcntl.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<sys/types.h>

#include<string>
#include<thread>

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
    ::write(STDERR_FILENO, str.data(), str.size());
    ::fsync(STDERR_FILENO);
}

static inline std::string daemonLogName(const char* name)
{
    std::string file=".";
    file += std::string("/")
        + name
        + "." + nowPathStringForestall()
        + ".pid"  + boost::lexical_cast<std::string>(::getpid())
        + ".dlog";
    return file;
}

void Daemon::start(const char* name)
{
    GMacroUnUsedVar(name);
#if !defined(DEBUG) && defined(NDEBUG)

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

#endif
}

void Daemon::watch(int pid)
{
    int status=0;
    ::waitpid(pid, &status, 0);

    if(WIFEXITED(status))
    {
        int exitStat=WEXITSTATUS(status);
        switch(exitStat)
        {
            case MainExitCode::eCodeFailure:
            {
                daemonMsgWrite("work process exit with failure, watch process exit\n");
                std::exit(EXIT_SUCCESS);
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
                daemonMsgWrite("work process abort, rerun it\n");
                break;
        }
    }

    //子进程是非正常退出，再创建子进程
}


}

