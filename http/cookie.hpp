//  Copyright [2016] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cookie.hpp
//
//   Description:  cookie 处理类
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

#pragma once

#include<set>
#include<map>
#include<ctime>
#include<string>

namespace core
{

class HttpCookieEle
{
public:
    HttpCookieEle()=default;

    template<typename N, typename V>
    HttpCookieEle(N&& name, V&& val)
        :name_(std::forward<N>(name)), value_(std::forward<V>(val))
    {}

    void fromString(const std::string& s);
    std::string toString();

    void domainSet(const std::string& domain)
    {
        domain_=domain;
    }

    void pathSet(const std::string& path)
    {
        path_=path;
    }

    void expiresSet(std::time_t tm)
    {
        expires_=tm;
    }

    bool operator<(const HttpCookieEle& o) const
    {
        return name_<o.name_;
    }

    bool empty() const
    {
        return name_.empty();
    }

    const std::string& nameGet() const
    {
        return name_;
    }

    const std::string& valueGet() const
    {
        return value_;
    }

private:
    bool httpOnly_=true;
    std::time_t expires_=0;

    std::string name_;
    std::string value_;
    std::string domain_;
    std::string path_;
};

class HttpCookie
{
public:
    void insert(const HttpCookieEle& ele)
    {
        eles_.emplace(ele.nameGet(), ele);
    }

    void insert(HttpCookieEle&& ele)
    {
        auto name=ele.nameGet();
        eles_.emplace(std::move(name), std::move(ele));
    }

    void fromSetCookieString(const std::string& str);
    void fromCookieString(const std::string& str);

    template<typename T>
    auto find(const T& t) const
    {
        return eles_.find(t);
    }

    auto begin() const
    {
        return eles_.begin();
    }

    auto end() const
    {
        return eles_.end();
    }

private:
    std::map<std::string, HttpCookieEle> eles_;
};

}

