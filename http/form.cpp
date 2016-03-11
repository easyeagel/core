//  Copyright [2016] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  form.cpp
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

#include<core/http/form.hpp>
#include<boost/algorithm/string/trim.hpp>
#include<boost/algorithm/string/split.hpp>
#include<boost/algorithm/string/predicate.hpp>
#include<boost/algorithm/string/classification.hpp>

namespace core
{

void HttpForm::fromUrlEncode(const std::string& code)
{
    std::vector<std::string> pairs;
    boost::algorithm::split(pairs, code, boost::algorithm::is_any_of("&"), boost::algorithm::token_compress_on);
    for(auto& l: pairs)
    {
        boost::algorithm::trim(l);
        std::vector<std::string> kv;
        boost::algorithm::split(kv, code, boost::algorithm::is_any_of("="), boost::algorithm::token_compress_on);
        if(kv.size()!=2)
            continue;

        for(auto& v: kv)
            boost::algorithm::trim(v);

        if(kv[0].empty())
            continue;

        dict_[kv[0]]=kv[1];
    }
}

}

