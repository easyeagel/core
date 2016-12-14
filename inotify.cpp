//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  inotify.cpp
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

#include<cassert>

#include"server.hpp"
#include"inotify.hpp"

namespace core
{

INotify::INotify()
    :stream_(MainServer::get(), fdopen())
{}

INotify::~INotify()
{
    const int fd=stream_.native_handle();
    if(fd>=0)
        ::close(fd);
}

int INotify::fdopen()
{
    int ret=::inotify_init1(IN_NONBLOCK|IN_CLOEXEC);
    assert(ret>=0);
    return ret;
}

void INotify::watch(const std::string& path, Mask_t mask, CallBack&& call)
{
    MainServer::post([this, path, mask, call=std::move(call)]()
        {
            int const fd=stream_.native_handle();
            const int wd=::inotify_add_watch(fd, path.c_str(), mask);
            index_[path]=fd;
            dict_[wd]=Value{path, mask, std::move(call)};
        }
    );
}

void INotify::remove(const std::string& path)
{
    MainServer::post([this, path]()
        {
            auto itr=index_.find(path);
            if(itr==index_.end())
                return;

            auto const fd=itr->second;
            index_.erase(itr);

            auto val=dict_.find(fd);
            if(val==dict_.end())
                return;

            dict_.erase(val);
        }
    );
}

void INotify::start()
{
    MainServer::post([this]()
        {
            stream_.async_read_some(boost::asio::buffer(buffer_, sizeof(buffer_)),
                [this](const boost::system::error_code& ec, std::size_t nbyte)
                {
                    this->readSomeCall(ec, nbyte);
                }
            );
        }
    );
}

void INotify::readSomeCall(const boost::system::error_code& ec, std::size_t nbyte)
{
    if(ec || nbyte<=0)
        return;

    struct ::inotify_event const *event=nullptr;
    for(const char* pointer=buffer_;
        static_cast<std::size_t>(pointer-buffer_)<nbyte;
        pointer += sizeof(struct ::inotify_event) + event->len)
    {
        event = reinterpret_cast<const struct ::inotify_event*>(pointer);
        auto itr=dict_.find(event->wd);
        if(itr==dict_.end())
            continue;
        itr->second.call(event->name, event->mask);
    }

    if(stop_)
        return;

    stream_.async_read_some(boost::asio::buffer(buffer_, sizeof(buffer_)),
        [this](const boost::system::error_code& ec, std::size_t nbyte)
        {
            this->readSomeCall(ec, nbyte);
        }
    );
}

void INotify::stop()
{
    MainServer::post([this]()
        {
            stop_=true;
            ErrorCode ec;
            stream_.cancel(ec);
        }
    );
}

}

