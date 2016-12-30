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
#ifdef GMacroWindows
#include<windows.h>
#endif
#include<string>
#include<boost/filesystem.hpp>

namespace core
{

class OS
{
public:
    static boost::filesystem::path getProgramPath();
    static boost::filesystem::path getProgramDir();
};

#ifndef WIN32
class PregressOpen
{
public:
    PregressOpen(const char* cmd, const char* mode)
        :file_(::popen(cmd, mode))
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

    ~PregressOpen()
    {
        if(file_==nullptr)
            return;
        ::pclose(file_);
        file_=nullptr;
    }

private:
    FILE* file_=nullptr;
};
#endif

}

