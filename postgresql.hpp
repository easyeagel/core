//  Copyright [2016] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  postgresql.hpp
//
//   Description:  postgresql C++ 简单封装
//
//       Version:  1.0
//       Created:  2016年01月28日 14时44分09秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#pragma once

#include<map>
#include<string>
#include<vector>
#include<core/codec.hpp>
#include<postgresql/libpq-fe.h>

namespace core
{

class PostgreSQL;
class PGStatement;
class PGResult;

class PostgreSQL
{
    friend class PGStatement;
public:
    PostgreSQL();

    void begin();
    void commit();
    void rollback();

    void paramSet(const std::string& k, const std::string& v)
    {
        params_[k]=v;
    }

    bool good() const;
    bool bad() const
    {
        return !good();
    }

    void connect();

    void execute(const std::string& sql);
    void execute(PGStatement& st);
    void execute(PGStatement& st, PGResult& rlt);

    std::string message() const;

private:
    ::PGconn* pg_=nullptr;
    std::map<std::string, std::string> params_;
};

class PGTransaction
{
public:
    PGTransaction(PostgreSQL& pg)
        :pg_(pg)
    {
        pg_.begin();
    }

    void lost()
    {
        lost_=true;
    }

    ~PGTransaction()
    {
        if(lost_==false)
            pg_.commit();
        else
            pg_.rollback();
    }
private:
    bool lost_=false;
    PostgreSQL& pg_;
};

class PGStatement
{
public:
    PGStatement(const std::string& s)
        :sql_(s)
    {}

    template<typename V>
    void intBind(unsigned idx, const V& v)
    {
        resize(idx+1);

        pgFormat_[idx]=1;
        pgSizes_[idx]=sizeof(v);
        params_[idx]=core::encode(v);
    }

    void byteBind(unsigned idx, const void* dt, unsigned sz)
    {
        resize(idx+1);

        pgFormat_[idx]=1;
        pgSizes_[idx]=sz;
        params_[idx]=std::string(reinterpret_cast<const char*>(dt), sz);
    }

    void byteBind(unsigned idx, const std::string& s)
    {
        resize(idx+1);

        pgFormat_[idx]=1;
        pgSizes_[idx]=s.size();
        params_[idx]=s;
    }

    void textBind(unsigned idx, const std::string& s)
    {
        resize(idx+1);

        pgFormat_[idx]=0;
        pgSizes_[idx]=s.size();
        params_[idx]=s;
    }

    void execute(PostgreSQL& pg, PGResult& rlt);

    void set(const std::string& s)
    {
        sql_=s;
    }
private:
    void resize(unsigned s)
    {
        if(params_.size()>s)
            return;

        params_.resize(s);
        pgSizes_.resize(s);
        pgFormat_.resize(s);
        pgParams_.resize(s);
    }

private:
    std::string sql_;
    std::vector<std::string> params_;

    std::vector<int> pgSizes_;
    std::vector<int> pgFormat_;
    std::vector<const char*> pgParams_;
};

class PGResult
{
    friend class PGStatement;
public:
    bool isBusy() const;
    bool next();

    int nfields() const;
    int ntuples() const;

    const uint8_t* fieldGet(int idx) const;
    int fieldSizeGet(int idx) const;
    bool isNull(int idx) const;

    template<typename Int>
    Int intRead(int idx)
    {
        auto ptr=fieldGet(idx);
        const auto sz=fieldSizeGet(idx);
        assert(sz==sizeof(Int));
        if(sz!=sizeof(Int))
            return -1;

        Int ret=*ptr;
        for(auto p=ptr+1; p<ptr+sz; ++p)
        {
            ret <<= 8;
            ret |= *p;
        }

        return ret;
    }

    template<typename IntVal>
    IntVal intValRead(int idx)
    {
        if(isNull(idx))
            return IntVal();
        return intRead<typename IntVal::Value_t>(idx);
    }

    std::string valRead(int idx)
    {
        auto ptr=fieldGet(idx);
        const auto sz=fieldSizeGet(idx);
        return std::string(reinterpret_cast<const char*>(ptr), sz);
    };

    void discard();

    ~PGResult();

private:
    ::PGconn* pg_=nullptr;
    ::PGresult* rlt_=nullptr;
};

}

