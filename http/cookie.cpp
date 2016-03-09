//  Copyright [2016] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cookie.cpp
//
//   Description: cookie 处理类 
//
//       Version:  1.0
//       Created:  2016年03月08日 16时34分07秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<core/string.hpp>
#include<core/http/cookie.hpp>
#include<boost/algorithm/string/trim.hpp>
#include<boost/algorithm/string/split.hpp>
#include<boost/algorithm/string/predicate.hpp>
#include<boost/algorithm/string/classification.hpp>

namespace core
{

std::string HttpCookieEle::toString()
{
    std::string code;
    code+=name_;
    code+='=';
    code+=value_;

    if(expires_)
    {
        code+=';';
        const auto tm=*std::gmtime(&expires_);
        char buf[64];
        std::strftime(buf, sizeof(buf), " %a, %d %b %Y %X GMT;", &tm);
        code+="Expires=";
        code+=buf;
    }

    if(!path_.empty())
    {
        code+=';';
        code+="Path=";
        code+=path_;
    }

    if(!domain_.empty())
    {
        code+=';';
        code+="Domain=";
        code+=domain_;
    }

    if(httpOnly_)
    {
        code+=';';
        code+="HttpOnly";
    }

    return std::move(code);
}

void HttpCookieEle::fromString(const std::string& str)
{
    //name=value; Expires Path Domain HttpOnly
    std::vector<std::string> pairs;
    boost::algorithm::split(pairs, str, boost::algorithm::is_any_of(";"), boost::algorithm::token_compress_on);
    for(auto& l: pairs)
    {
        boost::algorithm::trim(l);
        std::vector<std::string> kv;
        boost::algorithm::split(kv, str, boost::algorithm::is_any_of("="), boost::algorithm::token_compress_on);
        if(kv.size()!=2)
            continue;

        for(auto& l: kv)
            boost::algorithm::trim(l);

        switch(core::stringHash(kv[0].cbegin(), kv[0].cend()))
        {
            case core::constHash("Expires"):
            case core::constHash("expires"):
            {
                std::time_t t=0;
                expiresSet(t);
                break;
            }
            case core::constHash("Path"):
            case core::constHash("path"):
                pathSet(kv[1]);
                break;
            case core::constHash("Domain"):
            case core::constHash("domain"):
                domainSet(kv[1]);
                break;
            case core::constHash("HttpOnly"):
            case core::constHash("httpOnly"):
                httpOnly_=true;
                break;
            default:
                name_=kv[0];
                value_=kv[1];
                break;
        }
    }
}

void HttpCookie::fromSetCookieString(const std::string& s)
{
    std::string str=boost::algorithm::trim_copy(s);

    HttpCookieEle ele;
    static const std::string head("Set-Cookie:");
    if(boost::algorithm::starts_with(str, head))
        ele.fromString(str.substr(head.size()));
    else
        ele.fromString(str);

    if(!ele.empty())
        eles_.emplace(ele.nameGet(), ele);
}

void HttpCookie::fromCookieString(const std::string& s)
{
    std::string str=boost::algorithm::trim_copy(s);

    static const std::string head("Cookie:");
    if(boost::algorithm::starts_with(str, head))
        str=str.substr(head.size());

    std::vector<std::string> pairs;
    boost::algorithm::split(pairs, str, boost::algorithm::is_any_of(";"), boost::algorithm::token_compress_on);
    for(auto& l: pairs)
    {
        boost::algorithm::trim(l);

        //只分解第一个 =
        std::vector<std::string> kv;
        auto const pos=l.find('=');
        if(pos==std::string::npos)
            continue;
        kv.emplace_back(l.substr(0, pos));
        kv.emplace_back(l.substr(pos+1));

        for(auto& t: kv)
            boost::algorithm::trim(t);

        HttpCookieEle ele(kv[0], std::move(kv[1]));
        eles_.emplace(std::move(kv[0]), std::move(ele));
    }
}

}

