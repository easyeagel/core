//  Copyright [2016] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  form.hpp
//
//   Description:  表单处理
//
//       Version:  1.0
//       Created:  2016年03月10日 14时02分04秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include<map>
#include<string>

namespace core
{

class HttpForm
{
public:
    bool empty() const
    {
        return dict_.empty();
    }

    void fromUrlEncode(const std::string& code);

    template<typename K>
    auto find(K&& k) const
    {
        return dict_.find(std::forward<K&&>(k));
    }

    auto end() const
    {
        return dict_.end();
    }

    auto begin() const
    {
        return dict_.begin();
    }

private:
    std::map<std::string, std::string> dict_;
};

}

