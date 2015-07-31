//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  file.cpp
//
//   Description:  文件上传
//
//       Version:  1.0
//       Created:  2015年07月31日 09时22分27秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<core/http/file.hpp>

namespace core
{

void HttpFile::mutipartBody(ErrorCode& ecRet, const HttpParser& hp, const Byte* bt, size_t nb)
{
    if(times_==0)
    {
        if(boundary_.empty())
        {
            boundary_=MutilpartData::headCheck(ecRet, hp);
            if(ecRet.bad())
                return;
        }

        mutipart_.boundarySet(boundary_);
        mutipart_.partCallSet([this](const MutilpartData::PartTrait& trait, const Byte* byte, size_t size)
            {
                mutipartCall(trait, byte, size);
            }
        );
    }

    ++times_;
    mutipart_.parse(ecRet, bt, nb);
}

void HttpFile::mutipartCall(const MutilpartData::PartTrait& trait, const Byte* bt, size_t nb)
{
    switch(trait.stat)
    {
        case core::MutilpartData::eStatusStart:
        {
            stream_.close();
            streamOpen_(trait, stream_);
            break;
        }
        default:
            break;
    }

    if(bt && nb>0)
        stream_.write(reinterpret_cast<const char*>(bt), nb);
}

SimpleFileDispatcher::SimpleFileDispatcher(const boost::filesystem::path& dir)
    :dir_(dir)
{
    fileDest_.streamOpenSet([this](const MutilpartData::PartTrait& , HttpFile::FileStream& stream)
        {
            char name[256];
            std::snprintf(name, sizeof(name), "iptop-%03u.pdf", fileIndex_++);
            boost::filesystem::path path=dir_/name;
            stream.open(path);
        }
    );
}

void SimpleFileDispatcher::headCompleteCall(ErrorCode& ec, const HttpParser& hp)
{
    boundary_=MutilpartData::headCheck(ec, hp);
}

void SimpleFileDispatcher::bodyCall(ErrorCode& ec, const HttpParser& hp, const char* bt, size_t nb)
{
    fileDest_.mutipartBody(ec, hp, reinterpret_cast<const Byte*>(bt), nb);
}

HttpResponseSPtr SimpleFileDispatcher::bodyCompleteCall(ErrorCode& ec, const HttpParser& hp)
{
    auto respones=std::make_shared<HttpResponse>(HttpResponse::eHttpOk);
    respones->commonHeadSet("Content-Type", "application/json;charset=utf-8");
    respones->commonHeadSet("Connection", "keep-alive");
    respones->bodySet(R"JSON(
        {
            "status": "OK"
        }
    )JSON");
    respones->cache();

    return respones;
}

}

