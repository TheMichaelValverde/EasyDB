//  Created by Michael Valverde
//  MIT Licensed Open Source Project

#ifndef SqliteAnywhere_EasyDBAPI_h
#define SqliteAnywhere_EasyDBAPI_h

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
        int InitializeTable( string tableName, vector<string> fieldList);
        int AddRecord(string tableName, vector<string> values);
        int AddRecords(string tableName, vector<vector<string>> records);
        vector<string> GetFieldNames(string tableName);
        vector<vector<string>> GetRecords(string tableName);
        vector<string> GetRecord(string tableName, string whereClause);
        int DeleteRecords(string tableName);
        int DeleteRecord(string tableName, string whereClause);
        
    private:
        sqlite3* db;
        int TryStep(sqlite3_stmt* &stmt, int t, int r);
        string GetInsertStatement(string tableName, unsigned long &fieldCount);
    };
}


#endif
