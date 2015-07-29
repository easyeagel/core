//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  httpService.hpp
//
//   Description:  http 服务器
//
//       Version:  1.0
//       Created:  2015年03月04日 09时47分32秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include<core/macro.hpp>
#include<core/service.hpp>
#include<core/http/httpSession.hpp>

namespace core
{

class HttpService: public core::ServiceT<HttpService, HttpIOUnit>
{
    typedef core::ServiceT<HttpService, HttpIOUnit> BaseThis;

public:
    typedef HttpSSession SessionType;
    GMacroBaseThis(HttpService);

};

}




