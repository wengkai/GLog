#ifndef RECORD_H
#define RECORD_H

#include <vector>
#include <string>
#include <map>
#include <cstdlib>
#include <cstring>
#include <iostream>

class GRecord : public std::map<std::string, std::string> 
{
    
public:
    static constexpr const char* RESOLVE_HEADERS[] = {"qso_date", "time_on", "call", "freq", "mode", "rst_rcvd", "rst_sent"};
    static constexpr std::size_t RESOLVE_HEADERS_COUNT = sizeof(RESOLVE_HEADERS) / sizeof(char *);

    template<typename Compare>
    static inline bool compare(const std::string& l, const std::string& r, const std::string& field, Compare comp) {
        if (field == "freq") {
            return comp(std::stod(l), std::stod(r));
        }
        if (field == "rst_rcvd" || field == "rst_sent") {
            return comp(std::stoi(l), std::stoi(r));
        }
        return comp(l, r);
    };

    static inline bool less(const GRecord& a, const GRecord& b, const std::vector<std::string>& fields) {
        for (auto& field : fields) {
            auto pa = a.find(field);
            auto pb = b.find(field);
            if (pa == a.end() && pb == b.end()) {
                return false;
            }
            if (pb == b.end()) {
                return true;
            }
            if (pa == a.end()) {
                return false;
            }
            if (compare(pa->second, pb->second, field, std::equal_to<>{})) {
                continue;
            }
            return compare(pa->second, pb->second, field, std::less<>{});
        }
        return false;
    };

};

template<typename Ostream>
inline Ostream& operator<<(Ostream& stream, const GRecord& r) {
    for (auto& pair : r) {
        if (pair.second.empty()) {
            continue;
        }
        stream << '<';
        stream << pair.first;
        stream << ':';
        stream << pair.second.size();
        stream << '>';
        stream << pair.second;
    }
    stream << "<eor>";
    return stream;
};

#endif