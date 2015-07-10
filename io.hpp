//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  ioBase.hpp
//
//   Description:  IO相关的基础类型
//
//       Version:  1.0
//       Created:  2013年12月13日 11时55分19秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================



#pragma once

#include<memory>
#include<ostream>
#include<functional>

#include"int.hpp"
#include"error.hpp"
#include"string.hpp"
#include"typedef.hpp"

namespace core
{


/**
 * @brief 协议版本
 */
class SoftVersion: public IntValueT<SoftVersion, SoftVersion_t>
{
    typedef IntValueT<SoftVersion, uint16_t> BaseThis;
public:
    GMacroBaseThis(SoftVersion)
};

/**
 * @brief 主机地址，也就是IPv4地址
 */
class HostAddress: public FixedStringT<18, uint8_t>
{
    typedef FixedStringT<18, uint8_t> BaseThis;
public:
    GMacroBaseThis(HostAddress)

    static bool check(const char* str, size_t len);

    static bool check(const std::string& str)
    {
        return check(str.c_str(), str.size());
    }

    static HostAddress fromUInt32(uint32_t addr);

    typedef std::function<void (const char* name, const HostAddress& add)> LocalAddressCall;

    ///读取当前主机所有IPv4地址，并通过回调返回
    static void localAddressGet(LocalAddressCall&& call);

    bool isLAN() const;
    bool isLocal() const;

    bool isReserved() const;

    bool isWAN() const
    {
        return !isLAN() && !isLocal();
    }

    uint32_t toUInt32() const;
};

/**
 * @brief 主机端口，一般情况就是一个无符号整数
 */
class HostPort: public IntValueT<HostPort, ProPort_t>
{
    typedef IntValueT<HostPort, uint32_t> BaseThis;
public:
    GMacroBaseThis(HostPort)
};

/**
 * @brief 主机端点，由IPv4与端口构成
 */
class HostPoint
{
public:
    HostPoint()=default;

    HostPoint(const HostAddress& addr, const HostPort& port)
        :port_(port), address_(addr)
    {}

    const HostAddress& addressGet() const
    {
        return address_;
    }

    const HostPort& portGet() const
    {
        return port_;
    }

    void addressSet(const HostAddress& addr)
    {
        address_=addr;
    }

    void portSet(const HostPort& port)
    {
        port_=port;
    }

    static HostPoint fromString(const std::string& str);

    static bool check(const char* str, size_t len);

    static bool check(const std::string& str)
    {
        return check(str.c_str(), str.size());
    }

    bool operator==(const HostPoint& o) const
    {
        return port_==o.port_ && address_==o.address_;
    }

    bool empty() const
    {
        return address_.empty() || port_.empty();
    }

    std::string toString() const;

private:
    HostPort port_;
    HostAddress address_;
};

static inline std::ostream& operator<<(std::ostream& stm, const HostPoint& hp)
{
    stm << hp.addressGet() << ":" << hp.portGet().valueGet();
    return stm;
}

}


