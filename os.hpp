//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  os.hpp
//
//   Description:  系统相关代码
//
//       Version:  1.0
//       Created:  2015年12月03日 17时56分53秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include<cstdio>
#include<string>
#include<boost/filesystem.hpp>
#undef max

namespace core
{

class OS
{
public:
    static boost::filesystem::path getProgramPath();
    static boost::filesystem::path getProgramDir();
};

class StdIO
{
public:
    void inReset(int fd);
    void outReset(int fd);
    void inReset(const char* filename);
    void outReset(const char* filename);
};

//popen 封装
class ProcessOpen
{
public:
    ProcessOpen(const char* cmd, const char* mode)
#ifndef WIN32
        :file_(::popen(cmd, mode))
#else
		:file_(::_popen(cmd, mode))
#endif
    {}

    size_t read(void* buf, size_t nb)
    {
        assert(file_);
        return ::fread(buf, 1, nb, file_);
    }

    bool empty() const
    {
        return file_==nullptr;
    }

    ~ProcessOpen()
    {
        if(file_==nullptr)
            return;
#ifndef WIN32
        ::pclose(file_);
#else
		::_pclose(file_);
#endif
        file_=nullptr;
    }

private:
    FILE* file_=nullptr;
};

//进程管理
class Process
{
    struct PInfo;

public:
    Process();
	Process(const std::string& cmd);
    Process(Process&& other);
    Process(const Process& other);

    ~Process();

    void cmd(const std::string& cmd)
    {
        cmd_=cmd;
    }

	Process& arg(const std::string& arg)
	{
		args_.push_back(arg);
        return *this;
	}

    void start();
	void kill();
    int exitCode() const;

    bool wait();
    bool timedWait(uint32_t mill);

    bool bad() const
    {
        return error_!=0;
    }

    bool good() const
    {
        return !bad();
    }

	int pid() const;

    template<typename Call>
    static void forkCallPush(Call&& call)
    {
        forkCall().push_back(std::move(call));
    }

    static int shell(const std::string& cmd, uint64_t time=std::numeric_limits<uint64_t>::max());

private:
    std::string cmd_;
    std::vector<std::string> args_;

    static auto& forkCall()
    {
        static std::vector<std::function<void ()>> gs;
        return gs;
    }

private:
    int error_=0;
    std::unique_ptr<PInfo> pinfo_;
};

}

