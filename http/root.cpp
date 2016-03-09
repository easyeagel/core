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

static const ExternInfo allowedExterns[]=
{
    { ".txt",   "text/plain; charset=utf-8"},
    { ".html",  "text/html; charset=utf-8"},
    { ".js",    "text/javascript; charset=utf-8"},
    { ".css",   "text/css; charset=utf-8"},

    { ".bmp",   "image/bmp"},
    { ".png",   "image/png"},
    { ".jpg",   "image/jpeg"},
    { ".jpeg",  "image/jpeg"},
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
    //一个绝对路径，同时以 / 结尾
    assert(rootPath_.is_absolute()
        && boost::algorithm::ends_with(rootPath_.generic_wstring(), bf::path("/").generic_wstring()));
}

HttpResponseSPtr FileRoot::get(std::string httpPath) const
{
    if(httpPath.empty())
        return nullptr;

    if(httpPath.back()=='/')
        httpPath += "index.html";

    boost::system::error_code ec;
    bf::path path=bf::canonical(rootPath_/httpPath, ec);
    if(ec
        || false==boost::algorithm::starts_with(path.generic_wstring(), rootPath_.generic_wstring())
    )
    {
        return nullptr;
    }

    //重定向到当前目录
    if(bf::is_directory(path))
        return HttpDispatch::redirectMove((httpPath+'/').c_str());

    if(!bf::is_regular_file(path))
        return nullptr;

    auto checked=extGet(path);
    if(checked==nullptr)
        return nullptr;

    auto const size=bf::file_size(path);
    if(size>eSizeLimited)
        return nullptr;

    bf::ifstream stm(path, std::ios::binary|std::ios::in);
    if(!stm)
        return nullptr;

    auto ret=std::make_shared<HttpResponse>(HttpResponse::eHttpOk);
    ret->commonHeadSet("Content-Type", checked->type);
    ret->commonHeadSet("Connection", "keep-alive");

    std::string body(static_cast<size_t>(size), '\0');
    stm.read(const_cast<char*>(body.data()), body.size());
    ret->bodySet(std::move(body));
    ret->cache();

    return ret;
}

const ExternInfo* FileRoot::extGet(const bf::path& path) const
{
    auto const& s=path.generic_string();
    for(auto& ext: allowedExterns)
    {
        if(boost::algorithm::iends_with(s, ext.ex))
            return &ext;
    }

    return nullptr;
}

}

