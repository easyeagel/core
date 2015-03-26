//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  kvStore.cpp
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

#include"kvStore.hpp"

#include"gccWaringDisable.hpp"

#include"leveldb/include/leveldb/c.h"
#include"leveldb/include/leveldb/cache.h"
#include"leveldb/include/leveldb/comparator.h"
#include"leveldb/include/leveldb/db.h"
#include"leveldb/include/leveldb/env.h"
#include"leveldb/include/leveldb/filter_policy.h"
#include"leveldb/include/leveldb/iterator.h"
#include"leveldb/include/leveldb/options.h"
#include"leveldb/include/leveldb/slice.h"
#include"leveldb/include/leveldb/status.h"
#include"leveldb/include/leveldb/table.h"
#include"leveldb/include/leveldb/table_builder.h"
#include"leveldb/include/leveldb/write_batch.h"

#include"gccWaringEnable.hpp"



namespace core
{

class KVStoreLeveldb: public KVStore
{
    static leveldb::Slice cast(const char* key, size_t keySize)
    {
        return leveldb::Slice(key , keySize);
    }

    static inline ErrorCode cast(const leveldb::Status& )
    {
        return ErrorCode();
    }

public:
    KVStoreLeveldb(const char* dir)
    {
        options_.create_if_missing = true;
        options_.write_buffer_size = 64*1024*1024;
        options_.block_cache=leveldb::NewLRUCache(128*1024);

        leveldb::Status status = leveldb::DB::Open(options_, dir, &db_);
    };

    void get(const char* key, size_t keySize, GetCall&& call) final
    {
        const auto& lkey=cast(key, keySize);
        std::string dest;
        leveldb::Status st=db_->Get(leveldb::ReadOptions(), lkey, &dest);
        call(cast(st), dest.data(), dest.size());
    }

    void set(ErrorCode& ec, const char* key, size_t keySize, const char* val, size_t valSize) final
    {
        const auto& lkey=cast(key, keySize);
        const auto& lval=cast(val, valSize);
        leveldb::Status st=db_->Put(leveldb::WriteOptions(), lkey, lval);
        ec=cast(st);
    }

    void remove(ErrorCode& ec, const char* key, size_t keySize) final
    {
        const auto& lkey=cast(key, keySize);
        leveldb::Status st=db_->Delete(leveldb::WriteOptions(), lkey);
        ec=cast(st);
    }
private:
    leveldb::DB* db_=nullptr;
    leveldb::Options options_;
};

KVStore::~KVStore()=default;

}

