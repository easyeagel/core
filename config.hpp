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
#include<core/thread.hpp>
#include<boost/filesystem.hpp>

namespace core
{

template<typename Break>
class KVSplit
{

};

class KVConfig
{
    typedef boost::lock_guard<boost::mutex> Lock;
public:
    KVConfig();
    typedef std::vector<std::string> Line;
    typedef std::map<std::string, Line> Dict;

    const Line& get(const std::string& k) const
    {
        Lock lock(mutex_);

        const auto itr=dict_.find(k);
        if(itr==dict_.end())
            return emptyLine_;
        return itr->second;
    }

    const Line& get(const std::string& k, const Line& d) const
    {
        Lock lock(mutex_);

        const auto itr=dict_.find(k);
        if(itr==dict_.end())
            return d;
        return itr->second;
    }

    void set(const std::string& k, const Line& line)
    {
        Lock lock(mutex_);
        dict_[k]=line;
    }

    void load(const boost::filesystem::path& file);

    template<typename Call>
    void foreach(Call&& call) const
    {
        Lock lock(mutex_);
        for(auto& kv: dict_)
            call(kv);
    }

private:
    Line emptyLine_;
    Dict dict_;

    mutable boost::mutex mutex_;
};

}

