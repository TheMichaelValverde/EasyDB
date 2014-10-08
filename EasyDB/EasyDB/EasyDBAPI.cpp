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

int EasyDB::InitializeDatabase(const string & dbName)
{
#ifdef __MAC_OS_X_VERSION_MAX_ALLOWED
    const char *path = getenv("HOME");
#else //windows
	CHAR path[MAX_PATH];
	SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path);
#endif
    string fp = std::string(path);
    fp = fp + "/"+dbName;
	return sqlite3_open_v2(fp.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
}

int EasyDB::InitializeDatabase(const string & dbName, const string & folderPath)
{
	string fp;
	if (folderPath.empty())
		fp = dbName;
	else
		fp = folderPath + "/" + dbName;
    return sqlite3_open_v2(fp.c_str(),&db,SQLITE_OPEN_READWRITE |
                           SQLITE_OPEN_CREATE,NULL);
}

int EasyDB::GetRecords(const string & tableName, vector<vector<string>> & records)
{
    sqlite3_stmt *statement;
    string query("SELECT * FROM "+tableName+";");
	int rc = sqlite3_prepare_v2(db, VALUE(query), -1, &statement, 0);
    if(rc == SQLITE_OK)
    {
        int cols = sqlite3_column_count(statement);
        int result = 0;
        while(true)
        {
            rc = sqlite3_step(statement);
            if(rc == SQLITE_ROW)
            {
                vector<string> values;
                for(int col = 0; col < cols; col++)
                {
					char * colText = (char*)sqlite3_column_text(statement, col);
					if (colText != NULL)
						values.push_back(colText);
                }
				records.push_back(values);
            }
            else
            {
                break;
            }
        }
        sqlite3_finalize(statement);
    }
    return rc;
}

int EasyDB::GetRecord(const string & tableName, const string & whereClause, vector<string> & record)
{
    sqlite3_stmt *statement;
    string query("SELECT * FROM "+tableName+" WHERE "+whereClause);
	int rc = sqlite3_prepare_v2(db, VALUE(query), -1, &statement, 0);
    if(rc == SQLITE_OK)
    {
        int cols = sqlite3_column_count(statement);
        int result = 0;
        while(true)
        {
            rc = sqlite3_step(statement);
            if(rc == SQLITE_ROW)
            {
                for(int col = 0; col < cols; col++)
                {
					char * colStr = (char*)sqlite3_column_text(statement, col);
					if ( colStr != NULL)
						record.push_back(colStr);
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
    return rc;
}


int EasyDB::GetRecord(const string & tableName, int rowIndex, vector<string> & record)
{
    auto vStr = std::to_string(rowIndex);
    return this->GetRecord(tableName, "RecordNumber = "+vStr, record);
}

int EasyDB::DeleteRecords(const string & tableName)
{
    int rc = 0;
	string dSql("DELETE FROM " + tableName + ";");
	rc = sqlite3_exec(db, VALUE(dSql), NULL, NULL, NULL);
	return rc;
}

int EasyDB::DeleteRecord(const string & tableName, const string & whereClause)
{
    int rc = 0;
	string dSql("DELETE FROM " + tableName + " WHERE "+whereClause+";");
	rc = sqlite3_exec(db, VALUE(dSql), NULL, NULL, NULL);
	return rc;
}

int EasyDB::GetFieldNames(const string & tableName, vector<string> & fieldNames)
{
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
			fieldNames.push_back(tmp);
			rc = TryStep(stmt, 100, 10);
		}
	}
	sqlite3_finalize(stmt);
	return rc;
}

int EasyDB::AddRecords(const string & tableName, vector<vector<string>> records)
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
int EasyDB::CreateTable(const string & tableName, vector<string> & fieldList, const bool & overwrite)
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
	string upperTableNanme(tableName);
	transform(upperTableNanme.begin(), upperTableNanme.end(), upperTableNanme.begin(), toupper);
	string dSql("DROP TABLE IF EXISTS " + upperTableNanme + ";");
	string zSql("CREATE TABLE IF NOT EXISTS " + upperTableNanme + " (RecordNumber INTEGER NOT NULL PRIMARY KEY ");
    for( auto rec : fieldList)
	{
		zSql = zSql + ", " + rec + " TEXT ";
	}
	zSql = zSql + ");";
	rc = sqlite3_exec(db, VALUE(dSql), NULL, NULL, NULL);
	rc = sqlite3_exec(db, VALUE(zSql), NULL, NULL, NULL);
	return rc;
}

int EasyDB::AddRecord(const string & tableName, vector<string> values)
{
  	int rc = 0;
	sqlite3_stmt* stmt;
	unsigned long fieldCount = 0;
	string zSql = this->GetInsertStatement(tableName, fieldCount);
    rc = sqlite3_prepare_v2(db, VALUE(zSql), LENGTH(zSql), &stmt, 0);
	if (SUCCESS(rc))
	{
		sqlite3_bind_text(stmt, 1, VALUE(tableName), LENGTH(tableName), SQLITE_TRANSIENT);
		auto numValues = 0u;
		if (values.size() <= fieldCount-1)
			numValues = values.size();
		else
			numValues = fieldCount;

		for (auto i = 0u; i < numValues; i++)
		{
			string item = values[i];
			if (item.size() == 0)
				sqlite3_bind_null(stmt, i+1);
			else
				sqlite3_bind_text(stmt, i+1, VALUE(item), LENGTH(item), SQLITE_TRANSIENT);
		}

		for (unsigned int i = numValues; i < fieldCount; i++)
		{
			string item = ""; 
			sqlite3_bind_text(stmt, i+1, VALUE(item), LENGTH(item), SQLITE_TRANSIENT);
		}

		rc = this->TryStep(stmt, 100, 10);
		rc = sqlite3_finalize(stmt);
	}
	return rc;
}

string EasyDB::GetInsertStatement(const string & tableName, unsigned long &fieldCount)
{
	vector<string> fieldNames;
	string tmp = "INSERT INTO " + tableName + " (";
	string zSql(tmp.begin(), tmp.end());
	this->GetFieldNames(tableName, fieldNames);
	for(auto name:fieldNames)
    {
        if(name.compare("RecordNumber")==0) continue;
        zSql.append(name + ", ");
    }
	zSql = zSql.substr(0, zSql.size() - 2);
	zSql.append(") VALUES (");
	zSql.append("?");
	fieldCount = static_cast<unsigned char>(fieldNames.size());
	if (fieldCount <= 2)
	{
		zSql.append(");");
		return zSql;
	}
	for (unsigned int i = 3; i < fieldCount; i++)
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
int EasyDB::TableExists(const string & tableName, bool &exists)
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


int EasyDB::AddIndex(const string & tableName, const string & columnName, const SortOrder & sortOrder)
{
    string zSql = "CREATE INDEX IDX_"+tableName+"_"+columnName+" on "+tableName+"("+columnName+");";
    int rc = sqlite3_exec(db, VALUE(zSql), NULL, NULL, NULL);
    return rc;
}

int EasyDB::RemoveIndex(const string & tableName, const string & columnName)
{
    string zSql = "DROP INDEX IDX_"+tableName+"_"+columnName+";";
    int rc = sqlite3_exec(db, VALUE(zSql), NULL, NULL, NULL);
    return rc;
}

int EasyDB::AddColumn(const string & tableName, const string & columnName)
{
	bool exists = false;
	int rc = TableExists(tableName, exists);
	if (SUCCESS(rc))
	{
		if (!exists)
			return rc;

		string zSql("ALTER TABLE " + tableName + " ADD " + columnName + " TEXT");
		rc = sqlite3_exec(db, VALUE(zSql), NULL, NULL, NULL);
	}
	return rc;
}

unsigned int EasyDB::GetNumColumns(const string & tableName)
{
	int cols = 0;
	sqlite3_stmt *statement;
	string query("SELECT * FROM " + tableName + ";");
	if (sqlite3_prepare_v2(db, VALUE(query), -1, &statement, 0) == SQLITE_OK)
		cols = sqlite3_column_count(statement);
	return cols;
}

unsigned int EasyDB::GetNumRows(const string & tableName)
{
	int rows = 0;
	sqlite3_stmt *statement;
	string query("SELECT COUNT(*) FROM " + tableName + ";");
	if (sqlite3_prepare_v2(db, VALUE(query), -1, &statement, 0) == SQLITE_OK)
	{
		while (sqlite3_step(statement) == SQLITE_ROW)
		{
			rows = sqlite3_column_int(statement, 0);
		}
	}
	return rows;
}

int EasyDB::DeleteTable(const string & tableName)
{
	string zSql("DROP TABLE " + tableName);
	return sqlite3_exec(db, VALUE(zSql), NULL, NULL, NULL);
}
