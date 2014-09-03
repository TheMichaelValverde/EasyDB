//  Created by Michael Valverde
//  MIT Licensed Open Source Project
//

#include "EasyDBAPI.h"
#include <thread>
#include <chrono>
#include <stdlib.h>
#include <stdio.h>

//windows required api
#ifndef __MAC_OS_X_VERSION_MAX_ALLOWED
  #include <Windows.h>
  #include <shlobj.h>
  #include <algorithm>
#endif

using namespace openS3;

//class implementation
//Default Constructor
EasyDB::EasyDB()
{
}

//Default Destructor
EasyDB::~EasyDB()
{
	if (this->db != NULL)
	{
		sqlite3_close_v2(db);
	}
}

int EasyDB::InitializeDatabase(string dbName)
{
#ifdef __MAC_OS_X_VERSION_MAX_ALLOWED
    const char *path = getenv("HOME");
#else //windows
	CHAR path[MAX_PATH];
	SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path);
#endif
        string fp = std::string(path);
        fp = fp + "/"+dbName;
        return sqlite3_open_v2(fp.c_str(),&db,SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,NULL);
}

int EasyDB::InitializeDatabase(string dbName, string folderPath)
{
    string fp = std::string(folderPath);
    fp = fp + "/"+dbName;
    return sqlite3_open_v2(fp.c_str(),&db,SQLITE_OPEN_READWRITE |
                           SQLITE_OPEN_CREATE,NULL);
}

vector<vector<string>> EasyDB::GetRecords(string tableName)
{
    sqlite3_stmt *statement;
    vector<vector<string> > results;
    string query("SELECT * FROM "+tableName+";");
    if(sqlite3_prepare_v2(db, VALUE(query), -1, &statement, 0) == SQLITE_OK)
    {
        int cols = sqlite3_column_count(statement);
        int result = 0;
        while(true)
        {
            result = sqlite3_step(statement);
            if(result == SQLITE_ROW)
            {
                vector<string> values;
                for(int col = 0; col < cols; col++)
                {
                    values.push_back((char*)sqlite3_column_text(statement, col));
                }
                results.push_back(values);
            }
            else
            {
                break;
            }
        }
        sqlite3_finalize(statement);
    }
    return results;
}

vector<string> EasyDB::GetRecord(string tableName, string whereClause)
{
    sqlite3_stmt *statement;
    vector<string> results;
    string query("SELECT * FROM "+tableName+" WHERE "+whereClause);
    if(sqlite3_prepare_v2(db, VALUE(query), -1, &statement, 0) == SQLITE_OK)
    {
        int cols = sqlite3_column_count(statement);
        int result = 0;
        while(true)
        {
            result = sqlite3_step(statement);
            if(result == SQLITE_ROW)
            {
                for(int col = 0; col < cols; col++)
                {
                    results.push_back((char*)sqlite3_column_text(statement, col));
                }
            }
            else
            {
                break;
            }
        }
        sqlite3_finalize(statement);
    }
    string error = sqlite3_errmsg(db);
    return results;
}


vector<string> EasyDB::GetRecord(string tableName, int rowIndex)
{
    auto vStr = std::to_string(rowIndex);
    return this->GetRecord(tableName, "RecordNumber = "+vStr);
}

int EasyDB::DeleteRecords(string tableName)
{
    int rc = 0;
	string dSql("DELETE FROM " + tableName + ";");
	rc = sqlite3_exec(db, VALUE(dSql), NULL, NULL, NULL);
	return rc;
}

int EasyDB::DeleteRecord(string tableName, string whereClause)
{
    int rc = 0;
	string dSql("DELETE FROM " + tableName + " WHERE "+whereClause+";");
	rc = sqlite3_exec(db, VALUE(dSql), NULL, NULL, NULL);
	return rc;
}

vector<string> EasyDB::GetFieldNames(string tableName)
{
    vector<string> fieldList;
	int rc = 0;
	sqlite3_stmt* stmt;
	string zSql("pragma table_info('" + tableName + "');");
	rc = sqlite3_prepare_v2(db, VALUE(zSql), -1, &stmt, 0);
	if (SUCCESS(rc))
	{
		rc = TryStep(stmt, 100, 10);
		while (rc == SQLITE_ROW)
		{
			const unsigned char* ptr = sqlite3_column_text(stmt, 1);
			string tmp = (char*)ptr;
			fieldList.push_back(tmp);
			rc = TryStep(stmt, 100, 10);
		}
	}
	sqlite3_finalize(stmt);
	return fieldList;
}

int EasyDB::AddRecords(string tableName, vector<vector<string>> records)
{
    int rc = 0;
	for(auto rec:records)
    {
       rc = AddRecord(tableName, rec);
    }
    return rc;
}

//Initialize a new table (overwriting old one if exists)
//Create table has default parameter of overwrite = true
int EasyDB::CreateTable(string tableName, vector<string> fieldList, bool overwrite)
{
    int rc = 0;
    if(!overwrite)
    {
        bool exists = false;
        rc = TableExists(tableName, exists);
        if(SUCCESS(rc))
        {
            if(exists)
                return SQLITE_OK;
        }
        else
            return rc;
    }
    //uppercase tablename
    transform(tableName.begin(),tableName.end(),tableName.begin(), toupper);
	string dSql("DROP TABLE IF EXISTS " + tableName + ";");
	string zSql("CREATE TABLE IF NOT EXISTS " + tableName + " (RecordNumber INTEGER NOT NULL PRIMARY KEY ");
    for( auto rec : fieldList)
	{
		zSql = zSql + ", " + rec + " TEXT ";
	}
	zSql = zSql + ");";
	rc = sqlite3_exec(db, VALUE(dSql), NULL, NULL, NULL);
	rc = sqlite3_exec(db, VALUE(zSql), NULL, NULL, NULL);
	return rc;
}

int EasyDB::AddRecord(string tableName, vector<string> values)
{
  	int rc = 0;
	sqlite3_stmt* stmt;
	unsigned long fieldCount = 0;
	string zSql = this->GetInsertStatement(tableName, fieldCount);
    rc = sqlite3_prepare_v2(db, VALUE(zSql), LENGTH(zSql), &stmt, 0);
	if (SUCCESS(rc))
	{
		sqlite3_bind_text(stmt, 1, VALUE(tableName), LENGTH(tableName), SQLITE_TRANSIENT);
		for (int i = 1; i < fieldCount; i++)
		{
			string item = values[i - 1];
			if (item.size() == 0)
				sqlite3_bind_null(stmt, i);
			else
				sqlite3_bind_text(stmt, i, VALUE(item), LENGTH(item), SQLITE_TRANSIENT);
		}
		rc = this->TryStep(stmt, 100, 10);
		rc = sqlite3_finalize(stmt);
	}
	return rc;
}

string EasyDB::GetInsertStatement(string tableName, unsigned long &fieldCount)
{
	vector<string> fieldNames;
	string tmp = "INSERT INTO " + tableName + " (";
	string zSql(tmp.begin(), tmp.end());
	fieldNames = this->GetFieldNames(tableName);
	for(auto name:fieldNames)
    {
        if(name.compare("RecordNumber")==0) continue;
        zSql.append(name + ", ");
    }
	zSql = zSql.substr(0, zSql.size() - 2);
	zSql.append(") VALUES (");
	zSql.append("?");
	fieldCount = fieldNames.size();
	for (int i = 3; i < fieldCount; i++)
	{
		zSql.append(",?");
	}
	zSql.append(",?);");
	return zSql;
}

int EasyDB::TryStep(sqlite3_stmt* &stmt, int t, int r)
{
	int rc = -1;
	for (int retries = 0; retries < r; retries++)
	{
		rc = sqlite3_step(stmt);
		if (rc != SQLITE_BUSY)
			break;
		std::this_thread::sleep_for (std::chrono::seconds(t));
	}
	return rc;
}

//Check if table exists
int EasyDB::TableExists(string tableName, bool &exists)
{
    exists = false;
    int rc = 0;
    //sql statement to check if table exist regardless of "case"
    string zSql = "SELECT name FROM sqlite_master WHERE type='table' AND name='"+tableName+"' COLLATE NOCASE";
    sqlite3_stmt *statement;
    rc = sqlite3_prepare_v2(db, VALUE(zSql), -1, &statement, 0);
    if(SUCCESS(rc))
    {
        int rc = sqlite3_step(statement);
        if(rc == SQLITE_ROW)
        {
          exists = true;
        }
        sqlite3_finalize(statement);
    }
    return rc;
}


int EasyDB::AddIndex(string tableName, string columnName, SortOrder sortOrder)
{
    string zSql = "CREATE INDEX IDX_"+tableName+"_"+columnName+" on "+tableName+"("+columnName+");";
    int rc = sqlite3_exec(db, VALUE(zSql), NULL, NULL, NULL);
    return rc;
}

int EasyDB::RemoveIndex(string tableName, string columnName)
{
    string zSql = "DROP INDEX IDX_"+tableName+"_"+columnName+";";
    int rc = sqlite3_exec(db, VALUE(zSql), NULL, NULL, NULL);
    return rc;
}
