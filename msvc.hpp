//  Copyright [2014] <lgb (LiuGuangBao)>
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

#ifndef MR_MSVC_HPP
#define MR_MSVC_HPP

#ifdef _MSC_VER
#include<string>

#define constexpr
#define alignas(Int)
#define noexcept(Val)

namespace std
{

}

#endif  // _MSC_VER



#endif  // MR_MSVC_HPP
