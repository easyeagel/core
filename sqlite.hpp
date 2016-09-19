//  Copyright [2016] <lgb (LiuGuangBao)>
//=====================================================================================
//
//      Filename:  sqlite.hpp
//
//   Description:  SQLite C++ 接口
//
//       Version:  1.0
//       Created:  2016年08月28日 17时33分31秒
//      Revision:  none
//      Compiler:  gcc
//
//        Author:  lgb (LiuGuangBao), easyeagel@gmx.com
//  Organization:  ezbty.org
//
//=====================================================================================
//

#ifndef SQLITE_HAS_CODEC
    #define SQLITE_HAS_CODEC 1
#endif
#include<cassert>
#include<cstring>
#include<string>
#include<memory>
#include<functional>
#include<core/sqlite/sqlite3.h>

namespace core
{

namespace sqlite
{

class Error
{
    public:
        int error() const
        {
            return error_;
        }

        bool good() const
        {
            return error_==SQLITE_OK;
        }

        bool bad() const
        {
            return !good();
        }


    protected:
        int error_=SQLITE_OK;
};

}

class SQLite;

/**
 * @brief SQL查询组装，这里实现了SQLite的SQL模板化语言
 */
class SQLiteQuery: public sqlite::Error
{
    friend class SQLite;
public:
    /**
     * @brief 准备构造一个SQL模板
     * @details
     *  @li insert into table values(:1, :2)，其中1、2是模板索引
     *
     * @param db SQLite句柄
     * @param tpl SQL模板，具体写法参考SQLite模板
     */
    void init(const SQLite& db, const std::string& tpl);
    SQLiteQuery() = default;

    SQLiteQuery(const SQLiteQuery&) = delete;
    SQLiteQuery& operator=(const SQLiteQuery&) = delete;

    void reset();
    void bindText(int index, const std::string& text);
    void bindBlob(int index, const std::string& text);
    void bindInt(int index, int val);
    void bindInt64(int index, int64_t val);
    void bindDouble(int index, double val);
    void bindNull(int index);

    ~SQLiteQuery();

private:
    ::sqlite3_stmt* getStmt() const
    {
        return stmt_;
    }

private:
    ::sqlite3_stmt* stmt_=nullptr;
};

/**
 * @brief SQLite句柄
 * @details
 *  @li 一个数据库一个文件，一个数据库内可以有多个表
 */
class SQLite: public sqlite::Error
{
    friend class SQLiteQuery;
public:
    /**
     * @brief SQL执行回调
     * @details
     *  @li SQL语言执行结果是一个二维表，由行列组成
     *  @li 该回调针对一个结果单元执行一次
     *  @li columnCount 指出结果表中一行有多少列
     *  @li columnIndex 指出当前回调处理的列索引，从 0 开始计数
     *  @li value 为该单元数据的字符化表示，根据需要可能要进行转换
     *  @li num value 指向内存尺寸
     */
    typedef std::function<void (int columnCount, int columnIndex, const char* value, size_t num)> ExecuteCall;

    SQLite() = default;
    SQLite(const SQLite&) = delete;
    SQLite& operator=(const SQLite&) = delete;

    /**
     * @brief 打开指定SQLite数据库，如果不存在则创建
     *
     * @param path 数据库路径
     */
    void open(const std::string& path, const std::string& pass="123456")
    {
        open(path.c_str(), pass.c_str());
    }

    /**
     * @brief 执行指定的查询，结果通过回调传递
     *
     * @param sql SQL语句
     * @param call 回调，@ref ExecuteCall
     */
    void execute(const std::string& sql, ExecuteCall&& call=ExecuteCall());

    /**
     * @brief 执行指定的查询，结果通过回调传递
     *
     * @param sql 之前准备好的调查结构
     * @param call 回调，@ref ExecuteCall
     */
    void execute(const SQLiteQuery& sql, ExecuteCall&& call=ExecuteCall());

    /**
     * @brief 关闭数据库
     */
    void close();

    ~SQLite();

    void transactionStart();
    void transactionCommit();

    bool tableExist(const std::string& name) const;

private:
    void sqliteStep(::sqlite3_stmt* stmt, ExecuteCall&& call);
    void open(const char* path, const char* pass);


private:
    ::sqlite3* db_=nullptr;
};


}

