//  Copyright [2016] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  postgresql.cpp
//
//   Description:  postgresql C++ 简单封装
//
//       Version:  1.0
//       Created:  2016年01月28日 14时50分39秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<vector>
#include"postgresql.hpp"

namespace core
{

PostgreSQL::PostgreSQL()=default;

void PostgreSQL::connect()
{
    std::vector<const char*> param;
    std::vector<const char*> value;
    for(auto& p: params_)
    {
        param.emplace_back(p.first.c_str());
        value.emplace_back(p.second.c_str());
    }

    param.emplace_back(nullptr);
    value.emplace_back(nullptr);

    pg_=::PQconnectdbParams(param.data(), value.data(), 0);
}

bool PostgreSQL::good() const
{
    return pg_ && CONNECTION_OK==::PQstatus(pg_);
}

std::string PostgreSQL::message() const
{
    return ::PQerrorMessage(pg_);
}

void PostgreSQL::execute(PGStatement& st, PGResult& rlt)
{
    st.execute(*this, rlt);
}

void PGStatement::execute(PostgreSQL& pg, PGResult& rlt)
{
    const auto sz=params_.size();
    for(unsigned i=0; i<sz; ++i)
        pgParams_[i]=params_[i].data();

    auto ret=::PQsendQueryParams(pg.pg_, sql_.c_str(), sz, nullptr, pgParams_.data(), pgSizes_.data(), pgFormat_.data(), 1);
    if(ret==0)
        return;

    ret=::PQsetSingleRowMode(pg.pg_);
    if(ret==0)
        return;

    rlt.pg_=pg.pg_;
}

bool PGResult::isBusy() const
{
    return ::PQisBusy(pg_)!=0;
}

bool PGResult::next()
{
    for(;;)
    {
        if(rlt_)
            ::PQclear(rlt_);

        rlt_=::PQgetResult(pg_);
        if(rlt_==nullptr)
            return false;

        if(::PQntuples(rlt_)>0)
            return true;
    }
}

PGResult::~PGResult()
{
    if(rlt_)
        ::PQclear(rlt_);
    rlt_=nullptr;
}

int PGResult::nfields() const
{
    if(rlt_)
        return ::PQnfields(rlt_);
    return 0;
}

int PGResult::ntuples() const
{
    if(rlt_)
        return ::PQntuples(rlt_);
    return 0;
}

const uint8_t* PGResult::fieldGet(int idx) const
{
    if(rlt_)
        return reinterpret_cast<const uint8_t*>(::PQgetvalue(rlt_, 0, idx));
    return nullptr;
}

int PGResult::fieldSizeGet(int idx) const
{
    if(rlt_)
        return ::PQgetlength(rlt_, 0, idx);
    return 0;
}

bool PGResult::isNull(int idx) const
{
    if(rlt_)
        return ::PQgetisnull(rlt_, 0, idx)!=0 ? true : false;
    return true;
}

}

