//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  value.cpp
//
//   Description:  统一值类型
//
//       Version:  1.0
//       Created:  2015年05月14日 11时37分33秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include"int.hpp"
#include"value.hpp"
#include"macro.hpp"

namespace core
{

void SimpleValue::init(const SimpleValue& other)
{
    switch(other.tag_)
    {
        case SimpleValue_t::eNone:
            return init();
        case SimpleValue_t::eInt:
            return init(other.as<int32_t>());
        case SimpleValue_t::eString:
            return init(other.as<std::string>());
        case SimpleValue_t::eIntList:
            return init(other.as<IntList>());
        case SimpleValue_t::eStringList:
            return init(other.as<StringList>());
        default:
            init();
    }
}

void SimpleValue::init(SimpleValue&& other)
{
    switch(other.tag_)
    {
        case SimpleValue_t::eNone:
            init();
            break;
        case SimpleValue_t::eInt:
            init(other.as<int32_t>());
            break;
        case SimpleValue_t::eString:
            init(std::move(other.as<std::string>()));
            break;
        case SimpleValue_t::eIntList:
            init(std::move(other.as<IntList>()));
            break;
        case SimpleValue_t::eStringList:
            init(std::move(other.as<StringList>()));
            break;
        default:
            init();
            break;
    }

    other.reset();
}

void SimpleValue::reset()
{
    switch(tag_)
    {
        case SimpleValue_t::eNone:
        case SimpleValue_t::eInt:
            break;
        case SimpleValue_t::eIntList:
        {
            destructe(as<IntList>());
            break;
        }
        case SimpleValue_t::eString:
        {
            destructe(as<std::string>());
            break;
        }
        case SimpleValue_t::eStringList:
        {
            destructe(as<StringList>());
            break;
        }
        default:
            break;
    }

    tag_=SimpleValue_t::eNone;
}

bool SimpleValue::operator==(const SimpleValue& o) const
{
    if(tag_!=o.tag_)
        return false;

    switch(tag_)
    {
        case SimpleValue_t::eNone:
            return true;
        case SimpleValue_t::eInt:
            return as<int32_t>()==o.as<int32_t>();
        case SimpleValue_t::eIntList:
            return as<IntList>()==o.as<IntList>();
        case SimpleValue_t::eString:
            return as<std::string>()==o.as<std::string>();
        case SimpleValue_t::eStringList:
            return as<StringList>()==o.as<StringList>();
        default:
            GMacroAbort("bug");
            return false;
    }
}

void SimpleValue::encode(std::string& dest) const
{
    dest.push_back(static_cast<char>(tag_));
    switch(tag_)
    {
        case SimpleValue_t::eNone:
            return;
        case SimpleValue_t::eInt:
        {
            IntCode::encode(dest, get<SimpleValue_t::eInt>());
            return;
        }
        case SimpleValue_t::eIntList:
        {
            const auto& list=get<SimpleValue_t::eIntList>();
            IntCode::encode(dest, static_cast<uint32_t>(list.size()));
            for(const auto& v: list)
                IntCode::encode(dest, v);
            return;
        }
        case SimpleValue_t::eString:
        {
            const auto& v=get<SimpleValue_t::eString>();
            IntCode::encode(dest, static_cast<uint32_t>(v.size()));
            dest += v;
            return;
        }
        case SimpleValue_t::eStringList:
        {
            const auto& list=get<SimpleValue_t::eStringList>();
            IntCode::encode(dest, static_cast<uint32_t>(list.size()));
            for(const auto& v: list)
            {
                IntCode::encode(dest, static_cast<uint32_t>(v.size()));
                dest += v;
            }
            return;
        }
        default:
            GMacroAbort("bug");
            return;
    }
}

void SimpleValue::decode(uint32_t& start, const std::string& src)
{
    assert(tag_==SimpleValue_t::eNone);

    tag_=static_cast<SimpleValue_t>(src[start++]);

    switch(tag_)
    {
        case SimpleValue_t::eNone:
            return;
        case SimpleValue_t::eInt:
        {
            int32_t v=0;
            IntCode::decode(start, src, v);
            start += sizeof(v);

            init(v);
            return;
        }
        case SimpleValue_t::eIntList:
        {
            uint32_t size=0;
            IntCode::decode(start, src, size);
            start += sizeof(size);

            IntList list(size);
            for(auto& v: list)
            {
                IntCode::decode(start, src, v);
                start += sizeof(v);
            }

            init(std::move(list));
            return;
        }
        case SimpleValue_t::eString:
        {
            uint32_t size=0;
            IntCode::decode(start, src, size);
            start += sizeof(size);

            init(src.substr(start, size));
            start += size;

            return;
        }
        case SimpleValue_t::eStringList:
        {
            uint32_t size=0;
            IntCode::decode(start, src, size);
            start += sizeof(size);

            StringList list(size);
            for(auto& v: list)
            {
                uint32_t vz=0;
                IntCode::decode(start, src, vz);
                start += sizeof(vz);

                v=src.substr(start, vz);
                start += vz;
            }

            init(std::move(list));
            return;
        }
        default:
            GMacroAbort("bug");
            return;
    }
}

}

