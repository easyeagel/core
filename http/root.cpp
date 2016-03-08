//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  root.cpp
//
//   Description:  根目录实现，监视指定目录
//
//       Version:  1.0
//       Created:  2015年04月03日 16时59分42秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<cassert>
#include<boost/filesystem.hpp>
#include<boost/filesystem/fstream.hpp>
#include<boost/algorithm/string/predicate.hpp>

#include<core/http/root.hpp>

namespace core
{

namespace bf=boost::filesystem;

struct ExternInfo
{
    const char* ex;
    const char* type;
};

static ExternInfo allowedExterns[]=
{
    {".html", "text/html; charset=utf-8"},
    {".txt" , "text/plain; charset=utf-8"}
};

FileRoot::FileRoot()
    :rootPath_(bf::current_path()/bf::path("/"))
{
    assert(rootPath_.is_absolute());
    assert(boost::algorithm::ends_with(rootPath_.generic_wstring(), std::wstring(L"/")));
}

void FileRoot::rootPathSet(const boost::filesystem::path& path)
{
    rootPath_=path;
    assert(rootPath_.is_absolute()
        && boost::algorithm::ends_with(rootPath_.generic_wstring(), bf::path("/").generic_wstring()));
}

HttpResponseSPtr FileRoot::get(std::string httpPath) const
{
    if(httpPath.empty())
        return nullptr;

    if(httpPath.back()=='/')
        httpPath += "index.html";

    const ExternInfo* checked=nullptr;
    for(auto& ext: allowedExterns)
    {
        if(boost::algorithm::iends_with(httpPath, ext.ex))
        {
            checked=&ext;
            break;
        }
    }

    if(checked==nullptr)
        return nullptr;

    boost::system::error_code ec;
    bf::path path=bf::canonical(rootPath_/httpPath, ec);
    if(ec
        || false==boost::algorithm::starts_with(path.generic_wstring(), rootPath_.generic_wstring())
        || false==bf::is_regular_file(path))
    {
        return nullptr;
    }

    auto const size=bf::file_size(path);
    if(size>eSizeLimited)
        return nullptr;

    bf::ifstream stm(path, std::ios::binary|std::ios::in);
    if(!stm)
        return nullptr;

    auto ret=std::make_shared<HttpResponse>(HttpResponse::eHttpOk);
    ret->commonHeadSet("Content-Type", checked->type);
    ret->commonHeadSet("Connection", "Close");

    std::string body(static_cast<size_t>(size), '\0');
    stm.read(const_cast<char*>(body.data()), body.size());
    ret->bodySet(std::move(body));
    ret->cache();

    return ret;
}

}

