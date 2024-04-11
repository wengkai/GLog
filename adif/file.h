#ifndef __FILE_H__
#define __FILE_H__

#include <vector>
#include <set>
#include <iostream>
#include "record.h"



enum MERGE_OPT {
    MERGE_ADD = 1,
    MERGE_REPLACE,
    MERGE_MERGE,
    MERGE_SKIP,
    MERGE_QUIT
};

namespace adif {

class File {
public:
    int load(istream& in);
    int save(ostream& out) const;
    bool add(const adif::Record& record);
    void print(ostream& out) const;
    int size(void) const { return records.size(); }
    const Record* find(const adif::Record& record);
    void remove(const adif::Record& record);
    void update(const adif::Record& orecord,const adif::Record& nrecord);
    void export_csv(ostream& out) const;

    vector<adif::Record> search_records_by_call(const string& call) const;
    vector<adif::Record> search_records_by_date(const string& sdate, const string& edate) const;

    using MergeCallback = MERGE_OPT (*)(const adif::Record& orig, const adif::Record& cur);
    int merge(const adif::File& file, MergeCallback opt_callback);
private:
    // std::vector<adif::Record> records;
    std::set<adif::Record> records;
};
}

#endif // __FILE_H__