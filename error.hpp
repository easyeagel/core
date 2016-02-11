//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  error.hpp
//
//   Description:  全局错误码
//
//       Version:  1.0
//       Created:  2013年03月11日 10时57分17秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#pragma once

#include<vector>
#include<algorithm>

#define BOOST_SYSTEM_NO_DEPRECATED
#include<boost/system/error_code.hpp>

#include"msvc.hpp"
#include"base.hpp"
#include"macro.hpp"

namespace core
{

namespace details
{
    typedef boost::system::error_code ECBase;
    typedef boost::system::error_category ECatBase;
}

class ErrorCode: public details::ECBase
{
    typedef details::ECBase Base;
public:
    template<typename... Args>
    ErrorCode(Args&&... args)
        :Base(std::forward<Args&&>(args)...)
    {}

#ifdef _MSC_VER
    ErrorCode(ErrorCode&& ec)
        :Base(std::move(static_cast<Base&>(ec)))
        ,what_(std::move(ec.what_))
    {}
#else
    ErrorCode(ErrorCode&& ec)=default;
#endif // _MSC_VER

    ErrorCode(const ErrorCode& ec)=default;

    template<typename... Args>
    ErrorCode(const std::string& msg, Args&&... args)
        :Base(std::forward<Args&&>(args)...), what_(msg)
    {}

    ErrorCode& operator=(const Base& other)
    {
        static_cast<Base&>(*this) = other;
        return *this;
    }

    ErrorCode& operator=(const ErrorCode& other)=default;

    bool bad() const
    {
        return 0!=static_cast<const Base&>(*this);
    }

    bool good() const
    {
        return !bad();
    }

    int valueGet() const
    {
        return value();
    }

    const std::string& what() const
    {
        return what_;
    }

    std::string messageGet() const
    {
        if(what_.empty())
            return message();
        return what_ + ':' + message();
    }

    std::string message() const;

private:
    std::string what_;
};

class CoreError: public core::details::ECatBase
{
    enum Snatch { eCodeSnatch=8 };

    CoreError()=default;
public:
    ~CoreError();

    static const CoreError& instance();

    enum Code_t
    {
        eGood=0,
        eGroupDone,

        eBadStart=64,
        eTimerStopping, //定时器停止中
        eNetConnectError,
        eLogicError,
        eNetProtocolError,
        eObjectNotFound,
        eMemberIsFound,
        eMemberInfoDup, //用户信息重复

        eEnumCount
    };

    static ErrorCode ecMake(Code_t ec)
    {
        return ErrorCode(ec, instance());
    }

    static ErrorCode ecMake(Code_t ec, const std::string& msg)
    {
        return ErrorCode(msg, ec, instance());
    }

    template<typename Value>
    static ErrorCode ecMake(Value ec)
    {
        return ErrorCode(static_cast<Code_t>(ec), instance());
    }

    template<typename Value>
    static ErrorCode ecMake(Value ec, const std::string& msg)
    {
        return ErrorCode(msg, static_cast<Code_t>(ec), instance());
    }

    template<typename Value>
    static bool good(Value val)
    {
        return !bad(val);
    }

    template<typename Value>
    static bool bad(Value val)
    {
        return val>static_cast<Value>(eBadStart);
    }

    const char* name() const noexcept(true)
    {
        return "ezshError";
    }

    std::string message(int ec) const;

    struct Unit
    {
        Code_t ec;
        const char* msg;
    };

    static const std::vector<Unit> unitDict_;
};

template<typename Base=NullClass>
class ErrorBaseT: public Base
{
public:
    bool good() const
    {
        return ec_.good();
    }

    bool bad() const
    {
        return ec_.bad();
    }

    ErrorCode& ecGet()
    {
        return ec_;
    }

    const ErrorCode& ecGet() const
    {
        return ec_;
    }

    const ErrorCode& ecReadGet() const
    {
        return ec_;
    }

    const ErrorCode& ecReadGet()
    {
        return ec_;
    }

    details::ECBase& secGet()
    {
        return static_cast<details::ECBase&>(ec_);
    }

    const details::ECBase& secGet() const
    {
        return static_cast<const details::ECBase&>(ec_);
    }

    void ecSet(const ErrorCode& ec)
    {
        ec_=ec;
    }

    void ecClear()
    {
        ec_=ErrorCode();
    }

protected:
    ErrorCode ec_;
};

template<typename Obj, typename Base=NullClass>
class ErrorMemberT: public Base
{
public:
    bool good() const
    {
        return static_cast<const Obj*>(this)->ec().good();
    }

    bool bad() const
    {
        return static_cast<const Obj*>(this)->ec().bad();
    }

    ErrorCode& ecGet()
    {
        return static_cast<Obj*>(this)->ec();
    }

    const ErrorCode& ecGet() const
    {
        return static_cast<const Obj*>(this)->ec();
    }

    const ErrorCode& ecReadGet()
    {
        return static_cast<Obj*>(this)->ec();
    }

    const ErrorCode& ecReadGet() const
    {
        return static_cast<Obj*>(this)->ec();
    }

    details::ECBase& secGet()
    {
        return static_cast<details::ECBase&>(static_cast<Obj*>(this)->ec());
    }

    const details::ECBase& secGet() const
    {
        return static_cast<const details::ECBase&>(static_cast<const Obj*>(this)->ec());
    }

    void ecSet(const ErrorCode& ec)
    {
        static_cast<Obj*>(this)->ec()=ec;
    }

    void ecClear()
    {
        static_cast<Obj*>(this)->ec()=ErrorCode();
    }
};

typedef ErrorBaseT<> ErrorBase;

}

