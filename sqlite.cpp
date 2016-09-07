//  Copyright [2016] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  sqlite.cpp
//
//   Description:  SQLite C++ 接口
//
//       Version:  1.0
//       Created:  2016年08月28日 17时34分45秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#include<core/sqlite.hpp>
namespace core
{

void SQLiteQuery::init(const SQLite& db, const std::string& tpl)
{
    error_=::sqlite3_prepare_v2(db.db_, tpl.c_str(), tpl.size(), &stmt_, nullptr);
}

void SQLiteQuery::reset()
{
    if(stmt_)
        ::sqlite3_reset(stmt_);
}

void SQLiteQuery::bindText(int index, const std::string& text)
{
    error_=::sqlite3_bind_text(stmt_, index, text.data(), text.size(), SQLITE_TRANSIENT);
}

void SQLiteQuery::bindBlob(int index, const std::string& text)
{
    error_=::sqlite3_bind_blob(stmt_, index, text.data(), text.size(), SQLITE_TRANSIENT);
}

void SQLiteQuery::bindInt(int index, int val)
{
    error_=::sqlite3_bind_int(stmt_, index, val);
}

void SQLiteQuery::bindDouble(int index, double val)
{
    error_=::sqlite3_bind_double(stmt_, index, val);
}

void SQLiteQuery::bindNull(int index)
{
    error_=::sqlite3_bind_null(stmt_, index);
}

SQLiteQuery::~SQLiteQuery()
{
    ::sqlite3_finalize(stmt_);
}


void SQLite::execute(const std::string& sql, ExecuteCall&& call)
{
    ::sqlite3_stmt *stmt=nullptr;
    error_=::sqlite3_prepare_v2(db_, sql.c_str(), sql.size(), &stmt, nullptr);
    assert(good());
    if(bad())
        return;
    sqliteStep(stmt, std::move(call));
    ::sqlite3_finalize(stmt);
    stmt = nullptr;
}

void SQLite::execute(const SQLiteQuery& query, ExecuteCall&& call)
{
    sqliteStep(query.getStmt(), std::move(call));
}

void SQLite::sqliteStep(::sqlite3_stmt* stmt, ExecuteCall&& call)
{
    for(;;)
    {
        const int step=::sqlite3_step(stmt);
        switch(step)
        {
            case SQLITE_ROW:
            {
                int const columnCount=::sqlite3_column_count(stmt);
                for(int column=0; column<columnCount; ++column)
                {
                    const auto len=::sqlite3_column_bytes(stmt, column);
                    const auto ptr=::sqlite3_column_text(stmt, column);
                    if(call)
                        call(columnCount, column, reinterpret_cast<const char*>(ptr), len);
                }
                break;
            }
            case SQLITE_DONE:
            {
                error_=SQLITE_OK;
                return;
            }
            default:
            {
                error_=step;
                return;
            }
        }
    }
}

bool SQLite::tableExist(const std::string& name) const
{
    const std::string sql="SELECT name FROM sqlite_master WHERE type='table' AND name='" + name + "'";
    bool ret=false;
    const_cast<SQLite*>(this)->execute(sql,
        [&ret](int , int , const char*, size_t )
        {
            ret=true;
        }
    );
    return ret;
}

void SQLite::open(const char* path, const char* pass)
{
    error_ = ::sqlite3_open(path, &db_);
    if(bad())
        return;
    error_=::sqlite3_key(db_, pass, std::strlen(pass));
}

void SQLite::close()
{
    error_=::sqlite3_close(db_);
    db_=nullptr;
}

SQLite::~SQLite()
{
    if(db_)
        close();
}

void SQLite::transactionStart()
{
    execute("BEGIN;");
}

void SQLite::transactionCommit()
{
    execute("COMMIT;");
}




}

