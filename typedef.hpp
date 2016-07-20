//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  typedef.hpp
//
//   Description:  基本类型定义
//
//       Version:  1.0
//       Created:  2014年02月22日 10时22分49秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#pragma once


#include<cstdint>

namespace core
{


typedef uint32_t ProSize_t;    ///< 对象个数或尺寸

typedef uint16_t ProCmd_t;     ///< 所有命令类型
typedef uint16_t ProError_t;   ///< 错误码类型
typedef uint16_t SoftVersion_t; ///< 系统或软件版本号
typedef uint32_t ProPort_t;    ///< 端口号

typedef uint32_t ProUInt_t;
typedef uint8_t  ProUChar_t;
typedef uint16_t ProUShort_t;
typedef uint64_t ProUBigInt_t;

typedef uint8_t  Byte;

typedef int64_t  FileOff_t;  ///< 文件偏移
typedef uint64_t FileSize_t; ///< 文件尺寸

typedef uint32_t ObjectCount_t; ///< 对象个数的类型
typedef uint32_t ObjectIndex_t; ///< 对象索引类型

typedef uint32_t Counter_t; ///< 记数类型


typedef uint32_t MagicID_t; ///< 用户ID
typedef uint16_t FriendZone_t; ///< 用户分组

typedef uint16_t SvcZone_t;
typedef uint32_t SvnVersion_t;


typedef uint32_t GameScore_t;
typedef uint16_t GameRole_t;
typedef uint16_t GameLevel_t;

typedef uint32_t MessageRoomID_t;

typedef uint64_t SessionID_t;


}


