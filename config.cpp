//  Copyright [2016] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  config.cpp
//
//   Description:  配制文件处理
//
//       Version:  1.0
//       Created:  2016年01月29日 18时01分27秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<boost/filesystem/fstream.hpp>
#include<boost/algorithm/string/trim.hpp>
#include<boost/algorithm/string/split.hpp>
#include<boost/algorithm/string/predicate.hpp>

#include<core/config.hpp>

namespace core
{

KVConfig::KVConfig()=default;

void KVConfig::load(const boost::filesystem::path& file)
{
    boost::filesystem::ifstream stm(file);
    if(!stm)
        return;

    std::string line;
    while(std::getline(stm, line))
    {
        boost::trim(line);
        if(line.empty() || boost::starts_with(line, "#")) //注释
            continue;

        std::vector<std::string> kv;
        boost::split(kv, line, boost::is_any_of("="));
        if(kv.size()!=2) //解析出错，需要报告
            continue;

        boost::trim(kv[0]);
        boost::trim(kv[1]);
        if(kv[0].empty() || kv[1].empty())
            continue;

        std::vector<std::string> vals;
        boost::split(vals, kv[1], boost::is_any_of(","));

        for(auto& v: vals)
            boost::trim(v);

        dict_[std::move(kv[0])]=std::move(vals);
    }
}

}



