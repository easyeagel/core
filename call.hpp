//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  call.hpp
//
//   Description:  调用相关
//
//       Version:  1.0
//       Created:  2015年12月01日 16时29分05秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include<list>

namespace core
{

template<typename Call>
class CallListT
{
public:
    typedef std::list<Call> CallDict;

    template<typename C>
    void push(C&& call)
    {
        dict_.emplace_back(std::move(call));
    }

    template<typename... Args>
    void call(Args&&... args)
    {
        for(auto itr=dict_.begin(), end=dict_.end(); itr!=end; ++itr)
        {
            auto const beRemain=(*itr)(std::forward<Args&&>(args)...);
            if(beRemain==true)
                continue;
            itr=dict_.erase(itr);
        }
    }

private:
    CallDict dict_;
};

}

