//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  encode.cpp
//
//   Description:  编码实现
//
//       Version:  1.0
//       Created:  2015年01月25日 12时11分36秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include"encode.hpp"

#include<locale>
#include<boost/filesystem/detail/utf8_codecvt_facet.hpp>

namespace core
{

void utf8Enable(std::wios& io)
{
    std::locale old_locale;
    std::locale utf8_locale(old_locale, new boost::filesystem::detail::utf8_codecvt_facet);
    io.imbue(utf8_locale);
}

}

