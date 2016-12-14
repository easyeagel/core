//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  inotify.hpp
//
//   Description:  inotify 文件系统监视
//
//       Version:  1.0
//       Created:  2014年08月05日 10时28分48秒
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
#include<sys/inotify.h>
#include<boost/filesystem.hpp>
#include<boost/asio/posix/stream_descriptor.hpp>

namespace core
{

class INotify
{
public:
    INotify();
    ~INotify();

    typedef uint32_t Mask_t;
    typedef std::function<void (const char* path, Mask_t event)> CallBack;

    struct Value
    {
        std::string path;
        Mask_t mask;
        CallBack call;
    };

    enum Event_t:Mask_t
    {
        eAccese         =IN_ACCESS,
        eAttribute      =IN_ATTRIB,
        eWriteClose     =IN_CLOSE_WRITE,
        eNotWriteClose  =IN_CLOSE_NOWRITE,
        eCreate         =IN_CREATE,
        eDelete         =IN_DELETE,
        eDeleteSelf     =IN_DELETE_SELF,
        eModify         =IN_MODIFY,
        eMoveSelf       =IN_MOVE_SELF,
        eMovedFrom      =IN_MOVED_FROM,
        eMovedTo        =IN_MOVED_TO,
        eOpen           =IN_OPEN,
        eAll            =IN_ALL_EVENTS,
    };

    void watch(const std::string& path, Mask_t mask, CallBack&& call);
    void remove(const std::string& path);

    void start();
    void stop();

private:
    static int fdopen();
    void readSomeCall(const boost::system::error_code& ec, std::size_t nbyte);

private:
    bool stop_=false;
    char buffer_[1024];
    std::map<int, Value> dict_;
    std::map<std::string, int> index_;
    boost::asio::posix::stream_descriptor stream_;
};

}

