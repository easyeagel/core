//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  baseIO.cpp
//
//   Description:  IO基础类型的实现
//
//       Version:  1.0
//       Created:  2013年12月13日 13时19分33秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#include"io.hpp"

#include <ifaddrs.h>
#include <arpa/inet.h>

#include<algorithm>

#include<boost/xpressive/xpressive.hpp>
#include<boost/algorithm/string/predicate.hpp>

#include"thread.hpp"

namespace core
{


bool HostAddress::isLAN() const
{
    //10.x.x.x
    //172.16.x.x -> 172.31.x.x
    //192.168.x.x
    int dot=std::atoi(cstrGet());
    switch(dot)
    {
        case 10:
            return true;
        case 172:
        {
            const auto next=std::strchr(cstrGet(), '.')+1;
            dot=std::atoi(next);
            return dot>=16 && dot<=31;
        }
        case 192:
        {
            const auto next=std::strchr(cstrGet(), '.')+1;
            dot=std::atoi(next);
            return dot==168;
        }
        default:
            return false;
    }
}

bool HostAddress::isLocal() const
{
    //127.x.x.x
    int dot=std::atoi(cstrGet());
    switch(dot)
    {
        case 127:
            return true;
        default:
            return false;
    }
}

uint32_t HostAddress::toUInt32() const
{
    uint8_t dot=static_cast<uint8_t>(std::atoi(cstrGet()));
    uint32_t ret=dot;

    const char* str=std::strchr(cstrGet(), '.')+1;
    assert(str<cstrEnd());
    dot=static_cast<uint8_t>(std::atoi(str));
    ret=(ret<<8) | dot;

    str=std::strchr(str, '.')+1;
    assert(str<cstrEnd());
    dot=static_cast<uint8_t>(std::atoi(str));
    ret=(ret<<8) | dot;

    str=std::strchr(str, '.')+1;
    assert(str<cstrEnd());
    dot=static_cast<uint8_t>(std::atoi(str));
    ret=(ret<<8) | dot;

    return ret;
}

template<typename V>
V valueReserve(V v)
{
    std::reverse(reinterpret_cast<Byte*>(&v), reinterpret_cast<Byte*>(&v+1));
    return v;
}

void HostAddress::localAddressGet(LocalAddressCall&& call)
{
    struct ::ifaddrs *ifaddr=nullptr;
    if (::getifaddrs(&ifaddr) == -1)
        return ::freeifaddrs(ifaddr);

    for (auto ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family!=AF_INET)
            continue;
        const struct sockaddr_in* const sockaddr=reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
        call(ifa->ifa_name, fromUInt32(valueReserve(sockaddr->sin_addr.s_addr)));
    }

    ::freeifaddrs(ifaddr);
}

HostAddress HostAddress::fromUInt32(uint32_t addr)
{
    HostAddress ret;
    auto ptr=ret.ptrGet();
    auto endPtr=ptr+maxSizeGet();

    ptr += std::snprintf(ptr, endPtr-ptr, "%u", (addr>>24) & 0xFF);
    *ptr++ = '.';

    ptr += std::snprintf(ptr, endPtr-ptr, "%u", (addr>>16) & 0xFF);
    *ptr++ = '.';

    ptr += std::snprintf(ptr, endPtr-ptr, "%u", (addr>> 8) & 0xFF);
    *ptr++ = '.';

    ptr += std::snprintf(ptr, endPtr-ptr, "%u", (addr>> 0) & 0xFF);
    *ptr=0;

    ret.sizeSet(ptr-ret.ptrGet());

    return std::move(ret);
}

bool HostAddress::check(const char* str, size_t len)
{
    using namespace boost::xpressive;
    static OnceConstructT<cregex> gsIPv4(
        repeat<1,3>(_d) >> '.' >> repeat<1,3>(_d) >> '.' >> repeat<1,3>(_d) >> '.' >> repeat<1,3>(_d));
    return regex_match(str, str+len, gsIPv4.get());
}

HostPoint HostPoint::fromString(const std::string& hp)
{
    const auto pos=hp.find(':');
    if(pos==std::string::npos)
        return HostPoint();

    const char* start=hp.c_str();
    if(!HostAddress::check(start, pos))
        return HostPoint();

    const char* host=start;
    start += pos + 1;
    if(!boost::algorithm::all(start, [](char c){ return c>='0' && c<='9'; }))
        return HostPoint();

    return HostPoint(
        HostAddress(host, pos),
        HostPort(std::strtoul(start, nullptr, 10))
    );
}

std::string HostPoint::toString() const
{
    std::string ret=addressGet().toString();
    ret += ':';
    ret += std::to_string(portGet().valueGet());
    return std::move(ret);
}

bool HostPoint::check(const char* hp, size_t len)
{
    const char* end=hp+len;
    const auto pos=std::find(hp, end, ':');
    if(pos==end)
        return false;
    if(!HostAddress::check(hp, pos-hp))
        return false;
    return pos+1<end &&  std::all_of(pos+1, end, [](char c){ return c>='0' && c<='9'; });
}

}

