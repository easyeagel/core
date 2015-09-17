﻿/**
 * @file    Column.cpp
 * @ingroup SQLiteCpp
 * @brief   Encapsulation of a Column in a row of the result pointed by the prepared SQLite::Statement.
 *
 * Copyright (c) 2012-2014 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#include <SQLiteCpp/Column.h>

#include <iostream>


namespace SQLite
{


// Encapsulation of a Column in a row of the result pointed by the prepared Statement.
Column::Column(Statement::Ptr& aStmtPtr, int aIndex) :
    mStmtPtr(aStmtPtr),
    mIndex(aIndex)
{
}

// Finalize and unregister the SQL query from the SQLite Database Connection.
Column::~Column()
{
    // the finalization will be done by the destructor of the last shared pointer
}

// Return the named assigned to this result column (potentially aliased)
const char * Column::getName() const
{
    return sqlite3_column_name(mStmtPtr, mIndex);
}

#ifdef SQLITE_ENABLE_COLUMN_METADATA
// Return the name of the table column that is the origin of this result column
const char * Column::getOriginName() const
{
    return sqlite3_column_origin_name(mStmtPtr, mIndex);
}
#endif

// Return the integer value of the column specified by its index starting at 0
int Column::getInt() const
{
    return sqlite3_column_int(mStmtPtr, mIndex);
}

// Return the 64bits integer value of the column specified by its index starting at 0
sqlite3_int64 Column::getInt64() const
{
    return sqlite3_column_int64(mStmtPtr, mIndex);
}

// Return the double value of the column specified by its index starting at 0
double Column::getDouble() const
{
    return sqlite3_column_double(mStmtPtr, mIndex);
}

// Return a pointer to the text value (NULL terminated string) of the column specified by its index starting at 0
const char* Column::getText(const char* apDefaultValue /* = "" */) const
{
    const char* pText = reinterpret_cast<const char*>(sqlite3_column_text(mStmtPtr, mIndex));
    return (pText?pText:apDefaultValue);
}

// Return a pointer to the text value (NULL terminated string) of the column specified by its index starting at 0
const void* Column::getBlob() const
{
    return sqlite3_column_blob(mStmtPtr, mIndex);
}

// Return the type of the value of the column
int Column::getType() const
{
    return sqlite3_column_type(mStmtPtr, mIndex);
}

// Return the number of bytes used by the text value of the column
int Column::getBytes() const
{
    return sqlite3_column_bytes(mStmtPtr, mIndex);
}

// Standard std::ostream inserter
std::ostream& operator<<(std::ostream& aStream, const Column& aColumn)
{
    aStream << aColumn.getText();
    return aStream;
}


}  // namespace SQLite
