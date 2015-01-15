//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  base.cpp
//
//   Description:  base.hpp 实现定义
//
//       Version:  1.0
//       Created:  04/29/2013 04:49:03 PM
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================



#include"base.hpp"

#include<istream>
#include<ostream>

namespace core
{

const NullValueClass nullValue{};

std::ostream& operator<<(std::ostream& stm, const NullValueClass& )
{
    stm << "null";
    return stm;
}

}

