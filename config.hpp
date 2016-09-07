//  Copyright [2016] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  config.hpp
//
//   Description:  简单的KVl配置文件处理
//
//       Version:  1.0
//       Created:  2016年01月29日 18时00分59秒
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
#include<vector>
#include<boost/filesystem.hpp>

namespace core
{

template<typename Break>
class KVSplit
{

};

class KVConfig
{
public:
    KVConfig();
    typedef std::vector<std::string> Line;
    typedef std::map<std::string, Line> Dict;

    const Line& get(const std::string& k) const
    {
        const auto itr=dict_.find(k);
        if(itr==dict_.end())
            return emptyLine_;
        return itr->second;
    }

    void load(const boost::filesystem::path& file);

    auto begin() const
    {
        return dict_.begin();
    }

    auto end() const
    {
        return dict_.end();
    }

private:
    Line emptyLine_;
    Dict dict_;
};

}

