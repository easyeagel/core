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
#include<core/thread.hpp>

#include"http.hpp"

namespace ezweb
{

class FileRoot: public core::SingleInstanceT<FileRoot>
{
    FileRoot()=default;
    friend class core::OnceConstructT<FileRoot>;

    //最大不超过2M
    enum {eSizeLimited=1024*1024*2};
public:
    HttpResponseSPtr get(const std::string& ) const;

private:
    static const char* rootGet()
    {
        return "root";
    }

    const char* typeCheck() const;

};

}

