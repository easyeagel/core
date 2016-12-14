//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  cache.hpp
//
//   Description:  缓存同步技术
//
//       Version:  1.0
//       Created:  2014年07月05日 17时32分44秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#pragma once

#include<vector>
#include<memory>
#include<atomic>

#include<core/server.hpp>

namespace core
{

template<typename Value>
class MultiCacheT
{
public:
    typedef std::function<void (Value& dest)> Updater;

};

/**
 * @brief 一个写多个读，并且读不需要实时同步，如此允许缓存刚才的副本
 *
 * @tparam Value 缓存的类型
 */
template<typename Value>
class MultiSharedCacheT: public MultiCacheT<Value>
{
    typedef MultiCacheT<Value> BaseThis;
    typedef typename BaseThis::Updater Updater;
    typedef std::shared_ptr<Value> Unit;
public:
    MultiSharedCacheT()=default;

    /**
     * @brief 外部串行调用，用于刷新缓存内容
     */
    void refresh(Updater&& update)
    {
        assert(update);
        if(!update)
            return;

        for(auto& ptr: caches_)
        {
            if(ptr.use_count()>=2)
                continue;

            update(*ptr);
            std::atomic_exchange(&current_, ptr);
            return;
        }

        auto ptr=std::make_shared<Value>();
        update(*ptr);
        caches_.push_back(ptr);
        std::atomic_exchange(&current_, ptr);
        return;
    }

    Unit get() const
    {
        return std::atomic_load(&current_);
    }

private:
    Unit current_;
    std::vector<Unit> caches_;
};

/**
 * @brief 每个线程存在一个对象缓存
 * @note 本类实际上是一个单例类，因为它的所有对象共享缓存，
 *      也就是说，一个本类的实例只是一个访问入口，而访问的数据是共享的；
 *      如果想创建多个，请使用不同的 Index 值来实例本模板
 *
 * @tparam Value 对象类型
 * @tparam Index 创建多个缓存复本
 */
template<typename Value, size_t Index=0>
class MultiThreadCacheT: public MultiCacheT<Value>
{
    struct ThdValue
    {
        bool inited=false;
        Value value;
    };

    typedef MultiCacheT<Value> BaseThis;
    typedef typename BaseThis::Updater Updater;

public:
    void refresh(Updater&& update)
    {
        IOServer::postForeach([this, update]()
            {
                auto& inst=thdInst();
                update(inst.value);
                inst.inited=true;
            }
        );
    }

    const Value& get() const
    {
        const auto& inst=thdInst();
        return inst.value;
    }

private:
    static ThdValue& thdInst()
    {
        auto& inst=ThreadInstanceT<ThdValue>::instance();
        return inst;
    }
};

}

