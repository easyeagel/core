//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  root.hpp
//
//   Description:  根目录，允许在逻辑上不存在的路径到该目录查找
//
//       Version:  1.0
//       Created:  2015年04月03日 16时57分28秒
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

#include<core/thread.hpp>
#include<core/http/http.hpp>

namespace core
{

class FileRoot: public core::SingleInstanceT<FileRoot>
{
    FileRoot();
    friend class core::OnceConstructT<FileRoot>;

    //最大不超过2M
    enum {eSizeLimited=1024*1024*2};
public:
    void rootPathSet(const boost::filesystem::path& );
    HttpResponseSPtr get(std::string path) const;

private:
    boost::filesystem::path rootPath_;
};

}

