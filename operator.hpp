//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  operator.hpp
//
//   Description:  运算符辅助类
//
//       Version:  1.0
//       Created:  2014年09月09日 10时05分49秒
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

template<typename Object>
class TotalOrderT
{
    const Object&  objGet() const
    {
        return static_cast<const Object&>(*this);
    }
public:
    bool operator<=(const Object& other) const
    {
        return objGet()<other || objGet()==other;
    }

    bool operator>(const Object& other) const
    {
        return other<objGet();
    }

    bool operator>=(const Object& other) const
    {
        return other<objGet() || other==objGet();
    }

    bool operator!=(const Object& other) const
    {
        return !(other==objGet());
    }
};


}



