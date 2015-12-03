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

namespace core
{

static inline std::wstring getProgramDir()
{
#ifdef GMacroWindows
    wchar_t chpath[MAX_PATH];  
    GetModuleFileNameW(NULL,(LPSTR)chpath,sizeof(chpath));
#endif
}

}

