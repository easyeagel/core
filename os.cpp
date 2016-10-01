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

#include<core/os.hpp>

namespace core
{

boost::filesystem::path OS::getProgramPath()
{
    boost::filesystem::path ret;
#ifdef GMacroWindows
    wchar_t path[1024];  
    ::GetModuleFileNameW(NULL, path, sizeof(path));
    ret=path;
#elif defined(GMacroLinux)
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

}

