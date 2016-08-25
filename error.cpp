//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  error.cpp
//
//   Description:  错误码实现
//
//       Version:  1.0
//       Created:  2013年03月19日 11时41分15秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#include"msvc.hpp"
#include"error.hpp"
#include"encode.hpp"


#include<algorithm>

namespace core
{

std::string ErrorCode::message() const
{
    return Base::message();
}

const CoreError& CoreError::instance()
{
    static CoreError gs;
    return gs;
}

const std::vector<CoreError::Unit> CoreError::unitDict_
{
    {eGood,      u8"成功"},
    {eGroupDone, "eGroupDone"},

    {eBadStart,         "eBadStart"},

    { eTimerStopping,        u8"定时器正在停止"},
    { eNetConnectError,      u8"网络连接错误"},
    { eLogicError,           u8"逻辑错误"},
    { eNetProtocolError,     u8"网络协议错误"},
    { eObjectNotFound,       u8"对象不存在"},
    { eMemberIsFound,        u8"会员已存在"},
    { eMemberInfoDup,        u8"会员信息重复"},
    { eMemberIsNotFound,     u8"会员不存在"},
    { ePatternReplaceFailed, u8"模式替换失败"},
    { eCmdKeyTypeError,      u8"命令键数据类型错误"},
    { eUnkownDevice,         u8"未知设备"},
};

CoreError::~CoreError()=default;

std::string CoreError::message(int ec) const
{
    for(auto& u: unitDict_)
    {
        if(u.ec!=ec)
            continue;
        return u.msg;
    }
    return std::string(u8"未知错误");
}

}

