/*****************************************************************
 * Created by Michael Valverde
 * MIT Licensed Open Source Project
 * EasyDB is a simple wrapper class around SQLite database API.
 * Purpose is to make it really simple to create a database with
 * basic table(s) for quick storage and retrieval of data.
 *
 * With version 1.0 the following assumptions are made:
 * 1. All fields are TEXT fields int the database.
 * 2. All tables will have an autoincrementing primary key: 
 *      RecordNumber type int
 * 3. No secondary Indexes - so table scans at this point. 
 * 4. Number of records needs to be kept down since there are no
 *    secondary indexes
 ****************************************************************/
#ifndef EasyDBAPI_h
#define EasyDBAPI_h

#include <string>
#include "sqlite3.h"
#include <vector>

using namespace std;

#define SUCCESS(rc) (rc == SQLITE_OK || rc == SQLITE_OK || rc == SQLITE_ROW)

#define VALUE(val)  val.c_str()

#define LENGTH(val) (int)val.size()

enum SortOrder { Ascending = 1, Descending = 2 };


namespace openS3
{
    class EasyDB
    {
    public:
        EasyDB();
        ~EasyDB();
        int InitializeDatabase(const string & dbName);
        int InitializeDatabase(const string & dbName, const string & folderPath);
        int CreateTable(const string & tableName, vector<string> & fieldList, const bool & overwrite = true);
        int AddIndex(const string & tableName, const string & columnName, const SortOrder & sortOrder);
        int RemoveIndex(const string & tableName, const string & columnName);
        int AddRecord(const string & tableName, vector<string> values);
        int AddRecords(const string & tableName, vector<vector<string>> records);
		int GetFieldNames(const string & tableName, vector<string> & fieldNames);
		int GetRecords(const string & tableName, vector<vector<string>> & records);
		int GetRecord(const string & tableName, const string & whereClause, vector<string> & record);
		int GetRecord(const string & tableName, int rowIndex, vector<string> & record);
        int DeleteRecords(const string & tableName);
        int DeleteRecord(const string & tableName, const string & whereClause);
		int TableExists(const string & tableName, bool &exists);
        
    protected:
        sqlite3* db;
        int TryStep(sqlite3_stmt* &stmt, int t, int r);
        string GetInsertStatement(const string & tableName, unsigned long &fieldCount);
    };
}


#endif
