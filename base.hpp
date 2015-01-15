//  Copyright [2014] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  base.hpp
//
//   Description:  最基础的公共对象
//
//       Version:  1.0
//       Created:  2013年03月13日 11时20分11秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================


#ifndef MR_BASE_HPP
#define MR_BASE_HPP

#include<cstddef>
#include<cstdint>

#include<list>
#include<iosfwd>
#include<functional>
#include<type_traits>

#include"macro.hpp"
#include"typedef.hpp"

namespace core
{

class NullClass
{};

class NullValueClass
{};

class VoidCallBack
{
public:
    template<typename... Args>
    void operator()(Args&&... ) const
    {}

};

inline bool operator==(const NullValueClass& , const NullValueClass& )
{
    return true;
}

inline bool operator<(const NullValueClass& , const NullValueClass& )
{
    return false;
}

std::ostream& operator<<(std::ostream& stm, const NullValueClass& null);

extern const NullValueClass nullValue;

static inline bool sysIsBigEndian()
{
    return GMacroBigEndianIf(true, false);
}

template<typename T>
size_t byteWriteIf(bool sw, T v, Byte* bt)
{
    static_assert(std::is_arithmetic<T>::value, "Must int or float.");

    if(!sw)
    {
        *reinterpret_cast<T*>(bt)=v;
        return sizeof(T);
    }

    const Byte* ptr=reinterpret_cast<Byte*>(&v)+sizeof(T);
    for(size_t i=0; i<sizeof(T); ++i)
        *bt++=*--ptr;
    return sizeof(T);
}

template<typename T>
size_t byteReadIf(bool sw, const Byte* bt, T& v)
{
    static_assert(std::is_arithmetic<T>::value, "Must int or float.");

    if(!sw)
    {
        v=*reinterpret_cast<const T*>(bt);
        return sizeof(T);
    }

    Byte* ptr=reinterpret_cast<Byte*>(&v)+sizeof(T);
    for(size_t i=0; i<sizeof(T); ++i)
        *--ptr=*bt++;
    return sizeof(T);
}

template<typename T>
T byteReadIf(bool sw, const Byte* bt)
{
    T tmp;
    byteReadIf(sw, bt, tmp);
    return tmp;
}


template<typename T>
T compileConst(T v)
{
    return v;
}

template<typename T, typename C>
ptrdiff_t memoryDistance(const T* b, const C* e)
{
    return reinterpret_cast<const Byte*>(e)-reinterpret_cast<const Byte*>(b);
}

template<typename T>
T* memoryShift(T* b, ptrdiff_t sz)
{
    return reinterpret_cast<T*>(reinterpret_cast<Byte*>(b)+sz);
}

class ScopedCall
{
    ScopedCall(const ScopedCall&)=delete;
    ScopedCall& operator=(const ScopedCall&)=delete;
public:
    ScopedCall()=default;

    template<typename Fun>
    ScopedCall(Fun&& f)
        : callit_(true), calls_({std::move(f)})
    {}

    ScopedCall(ScopedCall&& sc)
        :callit_(true), calls_({std::move(sc.calls_)})
    {
        sc.callit_=false;
    }

    template<typename Fun>
    void push(Fun&& fun)
    {
        calls_.emplace_back(std::move(fun));
    }

    void notCallIt()
    {
        callit_=false;
    }

    ~ScopedCall()
    {
        if(callit_==false)
            return;

        for(auto& call: calls_)
            call();
    }

private:
    bool callit_=false;
    std::list<std::function<void()>> calls_;
};

template<typename Fun>
ScopedCall scopedCall(Fun&& fun)
{
    return std::move(fun);
}

template<typename T>
void destory(T* t)
{
    t->~T();
}

template<typename T>
static void fill(bool sw, T v, Byte* bt)
{
    if(sw)
    {
        const Byte* ptr=reinterpret_cast<Byte*>(&v)+sizeof(T);
        for(size_t i=0; i<sizeof(T); ++i)
            *bt++=*--ptr;
        return;
    }

    *reinterpret_cast<T*>(bt)=v;
    return;
}

template<typename T>
static T read(bool sw, const Byte* bt)
{
    if(sw)
    {
        T buf;
        Byte* ptr=static_cast<Byte*>(static_cast<void*>(&buf))+sizeof(T);
        for(size_t i=0; i<sizeof(T); ++i)
            *--ptr=*bt++;
        return buf;
    }

    return *reinterpret_cast<const T*>(bt);
}

}






#endif //MR_BASE_HPP

