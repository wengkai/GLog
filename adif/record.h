#ifndef RECORD_H
#define RECORD_H

#include <map>
#include <set>
#include <string>
#include <time.h>

using namespace std;

namespace adif {
class Record {
public:
    
    // load a record from a file, returns true if successful
    bool load(istream& in);
    // save a record to a file, returns true if successful
    bool save(ostream& out) const;

    // bool operator<(const Record& that) const {
    //     return date_time() < that.date_time();
    // }
    friend inline bool operator<(const Record& lhs, const Record& rhs);
    friend inline bool operator==(const Record& lhs, const Record& rhs);
    
    string to_string() const;

    time_t time_stamp() const;

    string date_time() const {
        if (fields.count("qso_date") > 0 && fields.count("time_on") > 0) {
            string ret= fields.at("qso_date") + fields.at("time_on");
            if ( ret.length() == 12 ) {
                ret += "00";
            }
            return ret;
        }
        return ""; // Handle the case when the required keys are not present
    }

    set<string> enum_fields(void) const;

    string get_field(const string& key) const {
        if (fields.count(key) > 0) {
            return fields.at(key);
        }
        return ""; // Handle the case when the key is not present
    }

    void set_field(const string& key, const string& value) {
        fields[key] = value;
    }

    void merge(const Record& that);
    static string& least_key_str(void); 
    string export_csv(set<string> fields) const;
    bool iscomplete() const;
private:
    map<string, string> fields; // Declare the fields member variable
    static const string least_keys[];
};

inline bool operator<(const Record& lhs, const Record& rhs) {
    if ( lhs.date_time() < rhs.date_time() )
        return true;
    if ( lhs.date_time() == rhs.date_time() )
        return lhs.get_field("call") < rhs.get_field("call");
    return false;
}

inline bool operator==(const Record& lhs, const Record& rhs) {
    return lhs.date_time() == rhs.date_time() &&
        lhs.get_field("call") == rhs.get_field("call");
}
}   //  namespace adif

#endif // RECORD_H