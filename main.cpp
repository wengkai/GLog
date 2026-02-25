// #include "file.h"
// #include "record.h"
// #include "logdb.h"

#include <iostream>
// #include <sqlite3.h>
#include <fstream>

#include "GLogApplication.h"
#include <QtWidgets/QApplication>

using namespace std;
// using namespace adif;

/*
enum MENU_ITEM {
    LOAD = 1,
    LIST,
    SEARCHDATE,
    SEARCHCALL,
    INPUT,
    EXPORT,
    QUIT
};
*/

static const string db_name = "alllog.adi";

/*
// LogDB theDB;
adif::File theDB;

int menu(void);

void load_db(void);
void save_db(void);

void load_adif(void);
void list(void);
void searchDate(void);
void searchCall(void);
void input(void);
void export_csv(void);
*/

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    auto window = new GLogApplication;
    window->show();
    return app.exec();
}

/*
void load_db(void)
{
    ifstream in(db_name);
    if ( in ) {
        theDB.load(in);
    }
    in.close();
}

void save_db(void)
{
    ofstream out(db_name);
    theDB.save(out);
    out.close();
}

int menu(void) 
{
    int ret = 0;
    do {
        cout << "1. Load (merge) an ADIF file" << endl;
        cout << "2. List all records" << endl;
        cout << "3. Search records by date" << endl;
        cout << "4. Search records by call" << endl;
        cout << "5. Input a record" << endl;
        cout << "6. Export all to a CSV file" << endl;
        cout << "7. Quit" << endl;
        cout << "Please enter your choice: ";
        if ( cin >> ret ) {
            if ( ret < 1 || ret > 7 ) {
                cout << "Invalid choice, please try again." << endl;
                ret = 0;
            }
        } else {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input, please try again." << endl;
        }
    } while ( ret ==0 );
    return ret;
}

void load_adif(void)
{
    string filename;
    cout << "Please enter the filename: ";
    cin >> filename;
    ifstream in(filename);
    if ( !in ) {
        cout << "Failed to open the file." << endl;
        return;
    }
    File f;
    f.load(in);
    cout << "Number of records loaded: " << f.size() << endl;
    //  save records into the db
    int cnt = theDB.merge(f, [](const Record& orig, const Record& cur) {
        cout << "Record already exists." << endl;
        cout << "Original: " << orig.to_string() << endl;
        cout << "New     : " << cur.to_string() << endl;
        cout << "(S)kip, (R)eplace, (A)dd, (M)erge, (Q)uit:";
        char choice;
        cin >> choice;
        switch (choice) {
        case 'A':
        case 'a':
            return MERGE_ADD;    
        case 'S':
        case 's':
            return MERGE_SKIP;
        case 'R':
        case 'r':
            return MERGE_REPLACE;
        case 'M':
        case 'm':
            return MERGE_MERGE;
        case 'Q':
        case 'q':
            return MERGE_QUIT;
        default:
            return MERGE_SKIP;
        }
    });
    save_db();
    cout << cnt << " record(s) has been added." << endl;
    in.close();
}

void list(void)
{
    theDB.print(cout);
    cout << "Number of records in database: " << theDB.size() << endl;
}

#include <ctime>

void input(void)
{
    static Record lastRecord;
    Record rec;
    char buffer[80];

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    string date;
    time_t rawtime;
    struct tm * timeinfo;
    time (&rawtime);
    timeinfo = gmtime(&rawtime);
    strftime(buffer,sizeof(buffer),"%Y%m%d",timeinfo);
    std::string str(buffer);
    std::cout << "date(" <<str<< "):";
    // cin >> date;
    getline(cin, date);
    if ( date.empty() ) {
        date = str;
    }
    rec.set_field("qso_date", date);

    string time;
    strftime(buffer,sizeof(buffer),"%H%M%S",timeinfo);
    str = buffer;
    std::cout << "time(" <<str<< "):";
    // cin >> time;
    getline(cin, time);
    if ( time.empty() ) {
        time = str;
    }
    rec.set_field("time_on", time);

    string freq;
    while ( freq.empty() ) {
        cout << "freq(" << lastRecord.get_field("freq") << ")";
        // cin >> freq;
        getline(cin, freq);
        if ( freq.empty() ) {
            freq = lastRecord.get_field("freq");
        }
    }
    rec.set_field("freq", freq);

    string mode;
    while ( mode.empty() ) {
        cout << "mode(" << lastRecord.get_field("mode") << ")";
        // cin >> mode;
        getline(cin, mode);
        if ( mode.empty() ) {
            mode = lastRecord.get_field("mode");
        }
    }
    rec.set_field("mode", mode);

    string call;
    cout << "call:";
    getline(cin, call);
    rec.set_field("call", call);

    string defrst = "59";
    if ( mode == "CW" || mode == "cw" ) {
        defrst = "599";
    }
    string rst_sent;
    cout << "rst_sent(" << defrst << "):";
    getline(cin, rst_sent);
    if ( rst_sent.empty() ) {
        rst_sent = defrst;
    }
    rec.set_field("rst_sent", rst_sent);

    string rst_rcvd;
    cout << "rst_rcvd(" << defrst << "):";
    getline(cin, rst_rcvd);
    if ( rst_rcvd.empty() ) {
        rst_rcvd = defrst;
    }
    rec.set_field("rst_rcvd", rst_rcvd);

    string name;
    cout << "name:";
    getline(cin, name);
    if ( !name.empty() ) {
        rec.set_field("name", name);
    }

    string qth;
    cout << "qth:";
    getline(cin, qth);
    if ( !qth.empty() ) {
        rec.set_field("qth", qth);
    }
    
    string comment;
    cout << "comment:";
    getline(cin, comment);
    if ( !comment.empty() ) {
        rec.set_field("comment", comment);
    }
    
    lastRecord = rec;
    theDB.add(rec);
    cout << rec.to_string() << endl;
    cout << "one record added." << endl;
    save_db();
}

void afterSearch(const vector<Record>& records);
void modify_record(Record& rec);

void searchDate(void)
{
    string sdate;
    cout << "The begin date(yyyymmdd):";
    cin >> sdate;
    string edate;
    cout << "The end date(yyyymmdd):";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, edate);
    if ( edate.empty() ) {
        edate = sdate;
    }
    vector<Record> records = theDB.search_records_by_date(sdate, edate);
    afterSearch(records);
}

void searchCall(void)
{
    string call;
    cout << "Please enter the callsign: ";
    cin >> call;
    vector<Record> records = theDB.search_records_by_call(call);
    afterSearch(records);
}

void export_csv(void)
{
    string filename;
    cout << "Please enter the csv filename: ";
    cin >> filename;
    ofstream out(filename);
    theDB.export_csv(out);
    out.close();
}

void afterSearch(const vector<Record>& records)
{
    int cnt = 1;
    for ( auto rec : records ) {
        cout << cnt << ": " << rec.to_string() << endl;
        cnt += 1;
    }
    cout << "Number of records found: " << records.size() << endl;
    if ( !records.empty() ) {
        char choice='Q';
        int num=1;
        while ( true) {
            cout << "(M)odify, (D)elete, (Q)uit:";
            cin >> choice;
            if ( choice == 'Q' || choice == 'q' ) {
                break;
            } else if ( choice != 'M' && choice != 'm' && choice != 'D' && choice != 'd' ) {
                continue;
            }
            cout << "Enter the number of the record:";
            cin >> num;
            if ( num < 1 || num > records.size() ) {
                cout << "Invalid number, please try again." << endl;
                continue;
            }
            break;
        }
        Record r = records[num-1];
        switch (choice) {
        case 'M':
        case 'm':
            modify_record(r);
            theDB.update(records[num-1], r);
            save_db();
            break;
        case 'D':
        case 'd':
            theDB.remove(r);
            save_db();
            break;
        }
    }
}

void modify_record(Record& rec)
{
    cout << "qso_date,time_on,freq,mode,call,rst_sent,rst_rcvd" << endl;
    cout << rec.to_string() << endl;
    cout << "qso_date and time_on can not be modified.\n";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    while (true) {
        string key;
        string value;
        cout << "key:" << endl;
        getline(cin, key);
        if ( key.empty() ) {
            break;
        }
        cout << "value:" << endl;
        getline(cin, value);
        if ( key != "qso_date" && key != "time_on" && !value.empty() ) {
            rec.set_field(key, value);
        }
    }
    cout << "Modified: " << rec.to_string() << endl;
}
*/