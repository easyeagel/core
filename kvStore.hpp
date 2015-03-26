//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  kvStore.hpp
//
//   Description:  key/value 数据库
//
//       Version:  1.0
//       Created:  2015年03月16日 14时46分37秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once
#include<memory>
#include<functional>

#include"error.hpp"
#include"sqlite/SQLiteCpp/SQLiteCpp.h"

namespace core
{


class KVStore
{
public:
    typedef std::function<void (const ErrorCode& ec, const char*, size_t)> GetCall;
    virtual void get(const char* key, size_t keySize, GetCall&& call) =0;

    virtual void set(ErrorCode& ec, const char* key, size_t keySize, const char* val, size_t valSize) =0;
    virtual void remove(ErrorCode& ec, const char* key, size_t keySize) =0;

    virtual void init(ErrorCode& ec) =0;

    static std::unique_ptr<KVStore> leveldbCreate(ErrorCode& ec, const char* dir);
    static std::unique_ptr<KVStore> sqliteCreate(ErrorCode& ec, SQLite::Database& db);

    template<typename Container>
    void get(ErrorCode& ec, const Container& key, Container& dest)
    {
        get(key.data(), key.size(),
            [&ec, &dest](ErrorCode& e, const char* val, size_t valSize)
            {
                ec=e;
                dest.assign(val, val+valSize);
            }
        );
    }

    template<typename Container>
    void set(ErrorCode& ec, const Container& key, const Container& val)
    {
        set(ec, key.data(), key.size(), val.data(), val.size());
    }

    template<typename Container>
    void remove(ErrorCode& ec, const Container& key)
    {
        remove(ec, key.data(), key.size());
    }

    virtual ~KVStore();
};



}




