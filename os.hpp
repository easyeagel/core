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
#include<boost/filesystem.hpp>

namespace core
{

static inline boost::filesystem::path getProgramPath()
{
    boost::filesystem::path ret;
#ifdef GMacroWindows
    wchar_t path[1024];  
    ::GetModuleFileNameW(NULL, path, sizeof(path));
    ret=path;
#endif
    return ret;
}

}

