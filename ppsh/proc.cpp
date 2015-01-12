//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  proc.cpp
//
//   Description:  Linux Proc 处理实现
//
//       Version:  1.0
//       Created:  2013年12月26日 15时06分34秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#include<sys/sysinfo.h>

#include<fstream>
#include<boost/filesystem.hpp>

#include"ppsh/proc.hpp"

namespace ppsh
{

/**
* @brief 商计算，如果有余数，则是加一
*
* @tparam T 被除数类型
* @tparam U 除数类型
* @param v 被除数
* @param t 除数
*
* @return 商或商加一
*/
template<typename T, typename U>
T quotient(T v, U t)
{
    T tmp=v%t;
    return v/t + (tmp?1:0);
}

/**
* @brief 四舍五入法除法
*
* @tparam T 被除数类型
* @tparam U 除数类型
* @param v 被除数
* @param t 除数
*
* @return 商，四舍五入(对余数)
*/
template<typename T, typename U>
T roundOff(T v, U t)
{
    auto q=v/t;
    auto r=v%t;

    return q + ((r>=quotient(t, 2)) ? 1 : 0);
}



bool ProcessInfo::refresh()
{
    std::string path="/proc/";
    path += std::to_string(pid_);
    path += "/stat";
    std::ifstream stat(path.c_str());
    if(!stat)
        return false;
    std::string line;
    if(!std::getline(stat, line))
        return false;

{
    int pid;
    char comm[256];
    char state;
    int ppid;
    int pgrp;
    int session;
    int tty_nr;
    int tpgid;
    unsigned flags;
    unsigned long minflt;
    unsigned long cminflt;
    unsigned long majflt;
    unsigned long cmajflt;
    unsigned long utime;
    unsigned long stime;
    long cutime;
    long cstime;
    long priority;
    long nice;
    long num_threads;
    long itrealvalue;
    unsigned long long starttime;
    unsigned long vsize;
    long rss;
    unsigned long rsslim;
    unsigned long startcode;
    unsigned long endcode;
    unsigned long startstack;
    unsigned long kstkesp;
    unsigned long kstkeip;
    unsigned long signal;
    unsigned long blocked;
    unsigned long sigignore;
    unsigned long sigcatch;
    unsigned long wchan;
    unsigned long nswap;
    unsigned long cnswap;
    int exit_signal;
    int processor;
    unsigned rt_priority;
    unsigned policy;
    unsigned long long delayacct_blkio_ticks;
    unsigned long guest_time;
    long cguest_time;

    int ret = std::sscanf(line.c_str(),
        "%d "   //pid      (1)
        "%s "   //comm     (2)
        "%c "   //state    (3)
        "%d "   //ppid     (4)
        "%d "   //pgrp     (5)
        "%d "   //session  (6)
        "%d "   //tty_nr   (7)
        "%d "   //tpgid    (8)
        "%u "   //flags    (9)
        "%lu "  //minflt  (10)
        "%lu "  //cminflt (11)
        "%lu "  //majflt  (12)
        "%lu "  //cmajflt (13)
        "%lu "  //utime   (14)
        "%lu "  //stime   (15)
        "%ld "  //cutime  (16)
        "%ld "  //cstime   (17)
        "%ld "  //priority   (18)
        "%ld "  //nice       (19)
        "%ld "  //num_threads  (20)
        "%ld "  //itrealvalue  (21)
        "%llu " //starttime  (22)
        "%lu "  //vsize    (23)
        "%ld "  //rss      (24)
        "%lu "  //rsslim   (25)
        "%lu "  //startcode  (26)
        "%lu "  //endcode  (27)
        "%lu "  //startstack  (28)
        "%lu "  //kstkesp  (29)
        "%lu "  //kstkeip  (30)
        "%lu "  //signal   (31)
        "%lu "  //blocked  (32)
        "%lu "  //sigignore (33)
        "%lu "  //sigcatch (34)
        "%lu "  //wchan    (35)
        "%lu "  //nswap    (36)
        "%lu "  //cnswap   (37)
        "%d "   //exit_signal  (38)
        "%d "   //processor  (39)
        "%u "   //rt_priority  (40)
        "%u "   //policy  (41)
        "%llu " //delayacct_blkio_ticks  (42)
        "%lu "  //guest_time  (43)
        "%ld "  //cguest_time  (44)
        , //======================================================================
        &pid,
        comm,
        &state,
        &ppid,
        &pgrp,
        &session,
        &tty_nr,
        &tpgid,
        &flags,
        &minflt,
        &cminflt,
        &majflt,
        &cmajflt,
        &utime,
        &stime,
        &cutime,
        &cstime,
        &priority,
        &nice,
        &num_threads,
        &itrealvalue,
        &starttime,
        &vsize,
        &rss,
        &rsslim,
        &startcode,
        &endcode,
        &startstack,
        &kstkesp,
        &kstkeip,
        &signal,
        &blocked,
        &sigignore,
        &sigcatch,
        &wchan,
        &nswap,
        &cnswap,
        &exit_signal,
        &processor,
        &rt_priority,
        &policy,
        &delayacct_blkio_ticks,
        &guest_time,
        &cguest_time
    );

    if(ret!=44)
        return false;

    assert(pid_==pid);

    cmd_=comm+1;
    cmd_.resize(cmd_.size()-1);

    auto now=std::time(nullptr);
    struct ::sysinfo sinfo;
    ::sysinfo(&sinfo);
    unsigned long long uptime=sinfo.uptime;

    static unsigned long tick=::sysconf(_SC_CLK_TCK);
    assert(uptime>=(starttime/tick));
    startTime_=now-(uptime-roundOff(starttime, tick));
}

    return true;
}

void SysProc::refresh()
{
    proc_.clear();
    boost::filesystem::directory_iterator beg("/proc");
    boost::filesystem::directory_iterator end;
    for(; beg!=end; ++beg)
    {
        std::string path=beg->path().filename().string();
        bool ispid=std::all_of(path.begin(), path.end(), [](char c){return c>='0' && c<='9';});
        if(!ispid)
            continue;
        ProcessInfo info(static_cast<PID_t>(std::stoul(path)));
        if(!info.refresh())
            continue;
        proc_.push_back(std::move(info));
    }
}

const ProcessInfo* SysProc::find(const std::string& name) const
{
    for(const auto& info: proc_)
    {
        if(info.cmdGet()==name)
            return &info;
    }

    return nullptr;
}


}


