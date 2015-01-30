﻿//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  msvc.hpp
//
//   Description:  MSVC 一些基本配制
//
//       Version:  1.0
//       Created:  2014年11月27日 09时50分30秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include<string>

#ifdef _MSC_VER

#define constexpr
#define alignas(Int)

#ifdef noexcept
    #undef noexcept
#endif
#define noexcept(Val)

namespace core
{

}

#endif  // _MSC_VER

