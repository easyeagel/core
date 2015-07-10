//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  log.cpp
//
//   Description:  easyDB 日志模块实现
//
//       Version:  1.0
//       Created:  04/26/2013 01:17:44 PM
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================



#include"time.hpp"
#include"log.hpp"
#include"server.hpp"
#include"string.hpp"

#include<cassert>
#include<ostream>
#include<iterator>

#include<boost/filesystem.hpp>
#include<boost/log/expressions.hpp>
#include<boost/log/attributes/clock.hpp>
#include<boost/log/support/date_time.hpp>
#include<boost/log/sinks/async_frontend.hpp>
#include<boost/log/expressions/formatters.hpp>
#include<boost/log/sinks/text_file_backend.hpp>
#include<boost/log/attributes/current_thread_id.hpp>
#include<boost/log/utility/setup/common_attributes.hpp>

#include<boost/iostreams/copy.hpp>
#include<boost/iostreams/device/file.hpp>
#include<boost/iostreams/filter/bzip2.hpp>
#include<boost/iostreams/filtering_stream.hpp>
#include<boost/iostreams/device/file_descriptor.hpp>

namespace core
{

bool GlobalLogInit::logOn_=true;
std::string GlobalLogInit::dir_="logs";
std::string GlobalLogInit::prefix_="svc.";

namespace io=boost::iostreams;
namespace bf=boost::filesystem;
namespace logging=boost::log;
namespace attrs=boost::log::attributes;
namespace src=boost::log::sources;
namespace sinks=boost::log::sinks;
namespace expr=boost::log::expressions;
namespace keywords=boost::log::keywords;

class BoostLog
{
    struct BZipSinkCollector: public sinks::file::collector
    {
        void store_file(bf::path const & path)
        {
            if(!ComputeServer::isStopped())
                return ComputeServer::post(std::bind(&BZipSinkCollector::bzipFile, this, path));
            bzipFile(path);
        }

        uintmax_t scan_for_files(sinks::file::scan_method , bf::path const& = bf::path(), unsigned int* = nullptr)
        {
            return 0;
        }

    private:
        void bzipFile(bf::path const& path)
        {
            std::ifstream in(path.c_str(), std::ifstream::in | std::ifstream::binary);
            if(!in)
                return;

            auto outPath=path;
            outPath += ".bz2";

            io::filtering_ostream out;
            out.push(io::bzip2_compressor());
            out.push(io::file_sink(outPath.string().c_str()));

            io::copy(in, out);

            bf::remove(path);
        }
    };

    typedef sinks::asynchronous_sink<sinks::text_file_backend> FileSink;

public:
    static void logginInit(const std::string& prefix)
    {
        logging::add_common_attributes();

        const std::streamsize oneSize=512*1024*1024;
        sinkPtr_.reset(new FileSink( keywords::file_name = prefix + "%Y%m%d%H%M%S.log"
            , keywords::rotation_size = oneSize));
        sinkPtr_->set_formatter(
            expr::format("%1%:%2%:%3%:%4% %5%")
                % expr::attr<unsigned int>("LineID")
                % expr::attr<attrs::current_thread_id::value_type>("ThreadID")
                % expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y%m%d%H%M%S")
                % logging::trivial::severity
                % expr::smessage
        );

        auto& backend=*sinkPtr_->locked_backend();
        backend.set_file_collector(
            boost::make_shared<BZipSinkCollector>()
        );

        backend.scan_for_files();

        auto core=logging::core::get();
        core->add_sink(sinkPtr_);
        severitySet(SeverityLevel::info);

        //定时刷日志
        static IOTimer timer(MainServer::serviceGet(), 100);
        timer.timerStart(
            [](const ErrorCode& )
            {
                ComputeServer::post(
                    []()
                    {
                        sinkPtr_->flush();
                    }
                );
            }
        );
    }

    static void loggOff()
    {
        logging::core::get()->set_logging_enabled(false);
    }

    static void severitySet(SeverityLevel level)
    {
        sinkPtr_->set_filter(
                logging::trivial::severity >= level
        );
    }

    static void autoFlushSet(bool val)
    {
        auto& backend=*sinkPtr_->locked_backend();
        backend.auto_flush(val);
    }

private:
    static boost::shared_ptr<FileSink> sinkPtr_;
};

boost::shared_ptr<BoostLog::FileSink> BoostLog::sinkPtr_;

GlobalLogInit::GlobalLogInit()
{
    struct Init
    {
        Init()
        {
            if(GlobalLogInit::logOffGet())
            {
                BoostLog::loggOff();
                return;
            }

            BoostLog::logginInit(dir_ + '/' + prefix_);
        }
    };

    static OnceConstructT<Init> gs;
}

void GlobalLogInit::init(const std::string& dir, const std::string& prefix)
{
    dir_=dir;
    prefix_=prefix;
    if(bf::exists(dir))
        return;
    auto ok=bf::create_directories(dir);
    if(!ok)
    {
        std::cerr << "Create logDir Failed: " << dir << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

SeverityLevel severityFromString(const char* lvl)
{
#ifndef _MSC_VER
    switch(constHash(lvl))
    {
        case constHash("trace"):
            return SeverityLevel::trace;
        case constHash("debug"):
            return SeverityLevel::debug;
        case constHash("info"):
            return SeverityLevel::info;
        case constHash("warning"):
            return SeverityLevel::warning;
        case constHash("error"):
            return SeverityLevel::error;
        case constHash("fatal"):
            return SeverityLevel::fatal;
        default:
            return SeverityLevel::info;
    }
#endif
    return SeverityLevel::info;
}

void GlobalLogInit::severitySet(SeverityLevel level)
{
    BoostLog::severitySet(level);
}

void GlobalLogInit::autoFlushSet(bool val)
{
    BoostLog::autoFlushSet(val);
}

}

