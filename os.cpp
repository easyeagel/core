//  Copyright [2016] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  os.cpp
//
//   Description:  系统设计相关的问题
//
//       Version:  1.0
//       Created:  2016年09月26日 14时36分34秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#ifdef WIN32
#include<io.h>
#include<stdio.h>
#include<Windows.h>
#else
#include<signal.h>
#include<sys/wait.h>
#endif

#include<cstdlib>
#include<cstdio>
#include<locale>
#include<codecvt>
#include<core/os.hpp>

namespace core
{

boost::filesystem::path OS::getProgramPath()
{
    boost::filesystem::path ret;
#ifdef WIN32
    wchar_t path[1024];  
    ::GetModuleFileNameW(NULL, path, sizeof(path));
    ret=path;
#else
    auto const pid=::getpid();
    const std::string link="/proc/"+std::to_string(pid)+"/exe";
    char path[1024];
    auto const sz=::readlink(link.c_str(), path, sizeof(path));
    if(sz>0)
        ret=std::string(path, sz);
#endif
    return ret;
}

boost::filesystem::path OS::getProgramDir()
{
    return getProgramPath().remove_filename();
}

void StdIO::inReset(int fd)
{
#ifdef WIN32
    ::_dup2(fd, ::_fileno(stdin));
#else
    ::dup2(fd, ::fileno(stdin));
#endif
}

void StdIO::outReset(int fd)
{
#ifdef WIN32
    ::_dup2(fd, ::_fileno(stdout));
#else
    ::dup2(fd, ::fileno(stdout));
#endif
}

void StdIO::inReset(const char* filename)
{
#ifdef WIN32
    ::freopen(filename, "r", stdin);
#else
    ::freopen(filename, "r", stdin);
#endif
}

void StdIO::outReset(const char* filename)
{
#ifdef WIN32
    ::freopen(filename, "w", stdout);
#else
    ::freopen(filename, "w", stdout);
#endif
}

#ifdef WIN32
struct Process::PInfo
{
    PInfo()
    {
        std::memset(&startup, 0, sizeof(startup));
        startup.cb = sizeof(startup);
        std::memset(&process, 0, sizeof(process));
    }

    ::STARTUPINFOW startup;
    ::PROCESS_INFORMATION process;

    ~PInfo()
    {
        if (process.hProcess)
        {
            ::CloseHandle(process.hProcess);
            ::CloseHandle(process.hThread);
        }
    }
};
#else
struct Process::PInfo
{
    int child=-1;
    int status=0;
};
#endif

Process::Process(const std::string& cmd)
	:cmd_(cmd)
    ,pinfo_(new PInfo)
{}

Process::Process()
    :pinfo_(new PInfo)
{}

Process::Process(Process&& other)=default;

Process::Process(const Process& other)
    :cmd_(other.cmd_)
    ,args_(other.args_)
    ,pinfo_(new PInfo)
{}

Process::~Process() = default;

void Process::start()
{
#ifdef WIN32
    std::wstring cmd;
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> conv;

    //生成命令
    cmd += L'"';
    cmd += conv.from_bytes(cmd_);
    cmd += L"\" ";
    for(auto& arg: args_)
    {
        auto const opt =conv.from_bytes(arg);
		bool const need = (opt.find(L' ') != std::wstring::npos);
		if(need)
			cmd += L'"';
		cmd += opt;
		cmd += (need ? L"\" " : L" ");
    }
    cmd.pop_back();

	assert(pinfo_ && pinfo_->process.hProcess==0);
    if (!::CreateProcessW(nullptr,
        const_cast<wchar_t*>(cmd.data()),  // Command line
        nullptr,       // Process handle not inheritable
        nullptr,       // Thread handle not inheritable
        true,          // Set handle inheritance to true
        0,             // No creation flags
        nullptr,       // Use parent's environment block
        nullptr,       // Use parent's starting directory 
        &pinfo_->startup, // Pointer to STARTUPINFO structure
        &pinfo_->process) // Pointer to PROCESS_INFORMATION structure
    ) 
    {
        error_=::GetLastError();
        return;
    }
#else
    std::vector<char*> cmdLine;
    cmdLine.push_back(const_cast<char*>(cmd_.c_str()));
    for(auto& arg: args_)
        cmdLine.push_back(const_cast<char*>(arg.c_str()));
    cmdLine.push_back(nullptr);

    const auto pid=::fork();
    switch(pid)
    {
        case 0://child
        {
            for(auto& call: forkCall())
                call();
            ::execvp(cmd_.c_str(), cmdLine.data());
            ::perror("execv");
            ::exit(EXIT_FAILURE);
        }
        case -1://error
        {
            error_=errno;
            break;
        }
        default:
        {
            pinfo_->child=pid;
            break;
        }
    }

#endif
}

void Process::kill()
{
#ifdef WIN32
	::TerminateProcess(pinfo_->process.hProcess, 0);
#else
    ::kill(pinfo_->child, SIGTERM);
    ::kill(pinfo_->child, SIGKILL);
#endif
}

int Process::exitCode() const
{
#ifdef WIN32
    DWORD code = 0;
    ::GetExitCodeProcess(pinfo_->process.hProcess, &code);
    return code;
#else
    return pinfo_->status;
#endif
}

bool Process::timedWait(uint32_t mill)
{
#ifdef WIN32
	assert(pinfo_ && pinfo_->process.hProcess!=0);
    const auto st=::WaitForSingleObject(pinfo_->process.hProcess, mill);
    return st==WAIT_OBJECT_0;
#else
    ::sigset_t mask;
    ::sigset_t oldMask;
    ::sigemptyset(&mask);
    ::sigaddset(&mask, SIGCHLD);
    ::pthread_sigmask(SIG_BLOCK, &mask, &oldMask);

    struct ::timespec timeout={ mill/1000,  (mill%1000)*1000*1000 };
    for(;;)
    {
        const auto ret=::sigtimedwait(&mask, nullptr, &timeout);
        if(ret>=0)
            break;

        switch(errno)
        {
            case EINTR: //中断
            {
                //处理时差后，再继续
                continue;
            }
            case EAGAIN: //超时
            default:
                return false;
        }
    }

    return wait();
#endif
}

bool Process::wait()
{
#ifdef WIN32
	assert(pinfo_ && pinfo_->process.hProcess!=0);
    const auto st=::WaitForSingleObject(pinfo_->process.hProcess, INFINITE);
    return st==WAIT_OBJECT_0;
#else
    for(;;)
    {
        const auto ret=::waitpid(pinfo_->child, &pinfo_->status, 0);
        if(ret>=0)
            return true;

        switch(errno)
        {
            case EINTR:
                continue;
            default:
                return false;
        }
    }
    return true;
#endif
}

int Process::pid() const
{
#ifdef WIN32
	assert(pinfo_ && pinfo_->process.hProcess!=0);
	return pinfo_->process.dwProcessId;
#else
    return pinfo_->child;
#endif
}

int Process::shell(const std::string& cmd, uint64_t time)
{
    core::Process shell("/bin/sh");
    shell
        .arg("-c")
        .arg(cmd);
    ;

    shell.start();
    auto const wret=shell.timedWait(time);
    if(wret==false)
    {
        shell.kill();
        shell.wait();
    }

    return shell.exitCode();
}

}

