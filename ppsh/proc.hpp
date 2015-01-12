//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  proc.hpp
//
//   Description:  Linux proc 文件系统
//
//       Version:  1.0
//       Created:  2013年12月26日 14时49分23秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#ifndef PPSH_PROC_HPP
#define PPSH_PROC_HPP

#include<ctime>
#include<string>
#include<vector>

namespace ppsh
{

typedef ::pid_t PID_t;

class ProcessInfo
{
public:
    ProcessInfo()=default;

    ProcessInfo(PID_t pid)
        :pid_(pid)
    {}

    PID_t pidGet() const
    {
        return pid_;
    }

    const std::string& cmdGet() const
    {
        return cmd_;
    }

    std::time_t startTimeGet() const
    {
        return startTime_;
    }

    bool refresh();

    static ProcessInfo cmdInfoRead(const std::string& cmd);

    bool empty() const
    {
        return pid_==0;
    }

private:
    PID_t pid_=0;
    std::string cmd_;
    std::time_t startTime_;
};

class SysProc
{
public:
    void refresh();
    const ProcessInfo* find(const std::string& name) const;

private:
    std::vector<ProcessInfo> proc_;
};

}

#endif //PPSH_PROC_HPP

