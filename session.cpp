//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  session.cpp
//
//   Description:  服务器框架会话接口
//
//       Version:  1.0
//       Created:  2013年04月07日 13时02分52秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#include"session.hpp"
#include"sessionPool.hpp"

namespace core
{

SSession::SSession()
    :BaseThis("SS")
{}

void SSession::sessionShutDown()
{
    BaseThis::sessionShutDown();
    SessionManager::free(shared_from_this());
}

SSession::~SSession()
{}

CSession::CSession()
    :BaseThis("CS")
{
}

CSession::~CSession()
{
    GMacroSessionLog(*this, SeverityLevel::info)
        << "ShutDown"
        ;
}


}

