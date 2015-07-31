//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  file.hpp
//
//   Description:  文件上传与处理
//
//       Version:  1.0
//       Created:  2015年07月31日 09时11分07秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once
#include<boost/filesystem.hpp>
#include<boost/filesystem/fstream.hpp>

#include<core/http/http.hpp>
#include<core/http/mutilpart.hpp>
#include<core/http/staticConst.hpp>

namespace core
{

class HttpFile
{
public:
    typedef boost::filesystem::ofstream FileStream;
    void mutipartBody(ErrorCode& ecRet, const HttpParser& hp, const Byte* bt, size_t nb);

    void boundarySet(const std::string& b)
    {
        boundary_=b;
    }

    typedef std::function<void (const MutilpartData::PartTrait&, FileStream& )> FileStreamOpen;

    void streamOpenSet(FileStreamOpen&& op)
    {
        streamOpen_=std::move(op);
    }

private:
    void mutipartCall(const MutilpartData::PartTrait& trait, const Byte* bt, size_t nb);

private:
    uint32_t times_=0;
    std::string boundary_;
    MutilpartData mutipart_;

    FileStream stream_;
    FileStreamOpen streamOpen_;
};

class SimpleFileDispatcher: public HttpDispatch::Dispatcher
{
public:
    SimpleFileDispatcher(const boost::filesystem::path& dir);

    void headCompleteCall(ErrorCode& ec, const HttpParser& hp) final;
    void bodyCall(ErrorCode& ec, const HttpParser& hp, const char* bt, size_t nb) final;
    HttpResponseSPtr bodyCompleteCall(ErrorCode& ec, const HttpParser& hp) final;

private:
    uint32_t fileIndex_=0;
    std::string boundary_;
    core::HttpFile fileDest_;
    boost::filesystem::path dir_;
};

}

