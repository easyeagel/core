//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  error.cpp
//
//   Description:  错误码实现
//
//       Version:  1.0
//       Created:  2013年03月19日 11时41分15秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#include"error.hpp"

#include<algorithm>

namespace core
{

std::string ErrorCode::message() const
{
#ifdef _MSC_VER
    return core::to(core::mbrtowc(Base::message()));
#else
    return Base::message();
#endif

}

}


