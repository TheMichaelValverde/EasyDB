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



namespace openS3
{
    class EasyDB
    {
    public:
        EasyDB();
        ~EasyDB();
        int InitializeDatabase(string dbName);
        int InitializeDatabase(string dbName, string folderPath);
        int CreateTable( string tableName, vector<string> fieldList, bool overwrite = true);
        int AddRecord(string tableName, vector<string> values);
        int AddRecords(string tableName, vector<vector<string>> records);
        vector<string> GetFieldNames(string tableName);
        vector<vector<string>> GetRecords(string tableName);
        vector<string> GetRecord(string tableName, string whereClause);
        vector<string> GetRecord(string tableName, int rowIndex);
        int DeleteRecords(string tableName);
        int DeleteRecord(string tableName, string whereClause);
        
    private:
        sqlite3* db;
        int TryStep(sqlite3_stmt* &stmt, int t, int r);
        string GetInsertStatement(string tableName, unsigned long &fieldCount);
        int TableExists(string tableName, bool &exists);
    };
}


#endif
