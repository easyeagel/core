//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  service.cpp
//
//   Description:  服务基础结构
//
//       Version:  1.0
//       Created:  2014年07月16日 10时42分13秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include"service.hpp"

#include<core/server.hpp>

namespace core
{

Service::Service()
    :timer_(core::MainServer::get())
{}


}  // namespace mr




