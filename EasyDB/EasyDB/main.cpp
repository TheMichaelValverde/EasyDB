//  Created by Michael Valverde
//  MIT Licensed Open Source Project
//
#include "EasyDBAPI.h"
#include <iostream>
#include <memory>
using namespace std;
using namespace openS3;

int main(int argc, const char * argv[])
{
    // insert code here...
    std::cout << "Hello, World!\n";
    unique_ptr<EasyDB> db(new EasyDB);
    db->InitializeDatabase("test.db");
//uncomment 1st time to create table
//    vector<string> fields;
//    fields.push_back("FirstName");
//    fields.push_back("LastName");
//    fields.push_back("PhoneNumber");
//    fields.push_back("Country");
//    //db->InitializeTable("TestTable", fields);
    
    cout<<"Adding a record!"<<endl;
    
    vector<string> record;
    record.push_back("Michael");
    record.push_back("Valverde");
    record.push_back("555555555");
    record.push_back("USA");
    
    db->AddRecord("TestTable",record);
    vector<string> rec = db->GetRecord("TestTable", "FirstName = 'Michael'");
    int y = 0;
    for(auto row:rec)
    {
        if(y%5!=0)
        cout<<row<<endl;
        else cout<<endl;
        y++;
    }
    
    
    cout<<"Adding records!"<<endl;
    cout<<endl;
    
    
    vector<vector<string>> records;
    rec.clear();
    rec.push_back("Jason");
    rec.push_back("Peek");
    rec.push_back("555-555-5555");
    rec.push_back("USA");
    records.push_back(rec);
    rec.clear();
    rec.push_back("Walter");
    rec.push_back("Perry");
    rec.push_back("555-555-5555");
    rec.push_back("USA");
    records.push_back(rec);
    rec.clear();
    rec.push_back("Jay");
    rec.push_back("Pearson");
    rec.push_back("555-555-5555");
    rec.push_back("USA");
    records.push_back(rec);
    db->AddRecords("TestTable",records);
    //example delete record
    //db->DeleteRecord("TestTable", "LastName = 'Valverde'");
    //db->DeleteRecords("TestTable");
    
    vector<vector<string>> recs = db->GetRecords("TestTable");
    for(auto row:recs)
    {
        int x = 1;
        for(auto col:row)
        {
            if(x>1)
            cout<<col<<endl;
            x++;
        }
        cout<<endl;
    }
    
    db->DeleteRecords("TestTable");
    
    return 0;
}

