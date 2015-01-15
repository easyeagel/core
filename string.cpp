//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  string.cpp
//
//   Description:  字符串相关实现
//
//       Version:  1.0
//       Created:  2013年06月07日 14时05分51秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#include<ctime>

#include"string.hpp"

namespace core
{

template class FixedStringT<6  , uint8_t>;
template class FixedStringT<14 , uint8_t>;
template class FixedStringT<18 , uint8_t>;
template class FixedStringT<30 , uint8_t>;
template class FixedStringT<46 , uint8_t>;
template class FixedStringT<62 , uint8_t>;
template class FixedStringT<126, uint8_t>;

template class FixedStringT<7>;
template class FixedStringT<15>;
template class FixedStringT<31>;
template class FixedStringT<63>;
template class FixedStringT<127>;

static const char dict[]= { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

void StringHex::encode(char const * input, char const* const end, char* dest)
{
    for(; input<end; ++input)
    {
        const uint8_t val=static_cast<uint8_t>(*input);
        const size_t up  =(val >> 4) & 0xf;
        const size_t down=(val >> 0) & 0xf;
        *dest++=dict[up];
        *dest++=dict[down];
    }
}

void StringHex::decode(char const * input, char const* const end, char* dest)
{
    assert((end-input)%2==0 && end>=input && dest!=nullptr);

    const auto& val2Index=[](uint8_t val) -> uint8_t
    {
        assert((val>='0' && val<='9') || (val>='A' && val<='F'));
        if(val>='0' && val<='9')
            return val-'0';
        if(val>='A' && val<='F')
            return val-'A'+10;
        return 0;
    };

    while(input<end)
    {
        const uint8_t v1=static_cast<uint8_t>(*input++);
        const uint8_t v2=static_cast<uint8_t>(*input++);
        *dest++ = (val2Index(v1) << 4) | val2Index(v2);
    }

}



}

