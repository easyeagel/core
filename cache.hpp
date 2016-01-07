//  Copyright [2015] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cache.hpp
//
//   Description:  缓存对象池
//
//       Version:  1.0
//       Created:  2015年12月16日 09时29分58秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once
#include<set>
#include<list>
#include<mutex>
#include<vector>
#include<memory>

namespace core
{

template<typename K, typename V>
class ObjectCachePoolT
{
public:
    typedef K Key;
    typedef V Value;

    enum { eDefaultCapacity=128*1024 };

    ObjectCachePoolT(size_t n=eDefaultCapacity)
        :capacity_(n)
    {}

    struct KVPair
    {
        Key key;
        Value value;
    };
    typedef std::shared_ptr<KVPair> CacheUnit;

    typedef std::list<CacheUnit> CacheList;
    typedef typename CacheList::iterator IndexUnit;

    struct IndexCompare
    {
        bool operator()(const IndexUnit& l, const IndexUnit& r) const
        {
            return (*l)->key<(*r)->key;
        }

        typedef Key is_transparent;

        bool operator()(const IndexUnit& l, const Key& r) const
        {
            return (*l)->key<r;
        }

        bool operator()(const Key& l, const IndexUnit& r) const
        {
            return l<(*r)->key;
        }
    };
    typedef std::set<IndexUnit, IndexCompare> CacheIndex;

    CacheUnit get(const Key& key)
    {
        auto itr=index_.find(key);
        if(itr==index_.end())
            return nullptr;
        moveToFront(*itr);
        return **itr;
    }

    template<typename KParam, typename VParam>
    auto set(KParam&& key, VParam&& val)
    {
        cacheSizeFix();

        cache_.emplace_front(
            std::make_shared<KVPair>(KVPair{std::forward<KParam&&>(key), std::forward<VParam&&>(val)}));
        auto itr=cache_.begin();
        auto rlt=index_.insert(itr);
        if(rlt.second)
            return **rlt.first;
        index_.erase(rlt.first);
        rlt=index_.insert(itr);
        return **rlt.first;
    }

    void clear()
    {
        index_.clear();
        cache_.clear();
    }

private:
    void moveToFront(const IndexUnit& u)
    {
        if(cache_.begin()==u)
            return;
        cache_.splice(cache_.begin(), cache_, u);
    }

    void cacheSizeFix()
    {
        if(cache_.size()<capacity_)
            return;
        auto back=--cache_.end();
        index_.erase(back);
        cache_.erase(back);
    }

private:
    size_t capacity_;
    CacheList cache_;
    CacheIndex index_;
};

template<typename K, typename V, typename H=std::hash<K>, typename M=std::mutex, typename L=std::unique_lock<M>>
class ObjectCacheSplicePoolT
{
public:
    typedef K Key;
    typedef V Value;
    typedef H Hash;
    typedef M Mutex;
    typedef L Lock;

    typedef ObjectCachePoolT<K, V> Pool;

    struct Unit
    {
        Unit(size_t n)
            :pool(n)
        {}

        Pool pool;
        Mutex mutex;
    };

    ObjectCacheSplicePoolT(size_t n, size_t np)
        :pools_(n)
    {
        for(auto& p: pools_)
            p=new Unit(np);
    }

    ~ObjectCacheSplicePoolT()
    {
        for(auto p: pools_)
            delete p;
        pools_.clear();
    }

    auto get(const Key& key)
    {
        auto& u=uget(key);
        Lock lock(u.mutex);
        return u.pool.get(key);
    }

    template<typename KParam, typename VParam>
    auto set(KParam&& k, VParam&& v)
    {
        auto& u=uget(k);
        Lock lock(u.mutex);
        return u.pool.set(std::forward<KParam&&>(k), std::forward<VParam&&>(v));
    }

private:
    Unit& uget(const Key& key)
    {
        const auto h=hash_(key);
        return *pools_[h%pools_.size()];
    }

private:
    Hash hash_;
    std::vector<Unit*> pools_;
};

}

