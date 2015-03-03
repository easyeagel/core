//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  log.hpp
//
//   Description:  easyDB 日志模块
//
//       Version:  1.0
//       Created:  04/26/2013 01:06:36 PM
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#ifndef MR_LOG_HPP
#define MR_LOG_HPP

#include"macro.hpp"
#include"thread.hpp"

#include<iosfwd>
#include<boost/log/trivial.hpp>
#include<boost/log/sources/severity_logger.hpp>


namespace core
{

typedef boost::log::trivial::severity_level SeverityLevel;

SeverityLevel severityFromString(const char* lvl);

class GlobalLogInit
{
    static constexpr const char* defaultLogPath()
    {
        return "svc.log";
    }

public:
    GlobalLogInit();

    static void init(const std::string& dir, const std::string& prefix);

    static void logOff()
    {
        logOn_=false;
    }

    static bool logOffGet()
    {
        return !logOn_;
    }

    static void severitySet(SeverityLevel level);

    static void autoFlushSet(bool val);

private:
    static bool logOn_;
    static std::string dir_;
    static std::string prefix_;
};

class Logger: private GlobalLogInit
{
    typedef boost::log::sources::severity_logger<SeverityLevel> LoggerImpl;
public:
    Logger()
        :log_(SeverityLevel::error)
    {}

    LoggerImpl& logGet()
    {
        return log_;
    }

    const LoggerImpl& logGet() const
    {
        return log_;
    }

private:
    LoggerImpl log_;
};

class ThreadLogger: public ThreadInstanceT<Logger>
{
};

#define GMacroThreadLog(CppLevel) \
    BOOST_LOG_SEV(ThreadLogger::instance().logGet(), CppLevel) \

}



#endif //MR_LOG_HPP

