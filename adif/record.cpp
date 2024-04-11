#include "record.h"
#include <iostream>
#include <algorithm>
#include <iomanip> //  gettime
#include <sstream> //  istringstream
#include <ctime>   //  gmtime

using namespace std;

// The least fileds that a record must have
const string adif::Record::least_keys[] = {
    "qso_date",
    "time_on",
    "freq",
    "mode",
    "call",
    "rst_sent",
    "rst_rcvd"};

#ifdef _FILE_TEST_
#include <iconv.h>

int gbk2utf8(const string& src, string& dst)
{
    iconv_t cd = iconv_open("UTF-8", "GBK");
    if (cd == (iconv_t)-1)
    {
        return -1;
    }
    size_t inlen = src.length();
    size_t outlen = inlen * 2;
    char *inbuf = (char *)src.c_str();
    char *outbuf = new char[outlen];//(char *)malloc(outlen);
    char *out = outbuf;
    if (outbuf == NULL)
    {
        return -1;
    }
    if (iconv(cd, &inbuf, &inlen, &out, &outlen) == (size_t)-1)
    {
        // free(outbuf);
        delete[] outbuf;
        return -1;
    }
    dst = string(outbuf, out - outbuf);
    // free(outbuf);
    delete[] outbuf;
    iconv_close(cd);
    return 0;
}
#endif

std::string& trim(std::string& in) 
{
    if (in.empty()) {
        return in;
    }
    in.erase(0, in.find_first_not_of(" \t\n\r"));
    in.erase(in.find_last_not_of(" \t\n\r") + 1);
    return in;
}

//<qso_date:8>20051128<time_on:4>1153<freq:5>21125<mode:2>CW<call:6>BG5HLY<rst_rcvd:3>599<rst_sent:3>599<my_rig:13>FT847 100W GP<eor>
/**
 * @brief load a record from a file, returns true if successful
 * The format of the record is:
 * <key1:length1>value1<key2:length2>value2<key3:length3>value3<eor>
 * The keys are to be lowercased for further processing.
 * The order of the keys is not important.
 * The record is not complete if any of the keys in least_keys is missing.
 * There may be other keys in the record.
 * The record is terminated by <eor>.
 * The record may be in multiple lines.
 * There may be spaces in the value, and/or after valid characters.
 * There is no space between the key and the colon.
 * There may be filed that has no value, i.e. <key:0>. This key is not to be read in in this case.
 * @param in the input stream
 * @return true if successful
 */
bool adif::Record::load(istream &in)
{
    // static int ln = 0;
    bool ret = false;
    string line;
    while (getline(in, line))
    {
        // cout << ln++ << ":" << line << endl;
        int last = 0; //  the beginning of the next field
        while (last < line.length())
        {
            size_t leftangle = 0;  //  location for the <
            size_t rightangle = 0; //  location for the >
            size_t colon = 0;      //  location for the : inside the <>
            leftangle = line.find('<', last);
            colon = line.find(':', last);
            rightangle = line.find('>', last);
            //  there must be these <:> things, or this line has nothing left
            if (leftangle == string::npos || rightangle == string::npos)
            {
                break;
            }
            if (colon == string::npos)
            {
                string key = line.substr(leftangle + 1, rightangle - leftangle - 1);
                transform(key.begin(), key.end(), key.begin(), ::tolower);
                if (key == "eor")
                {
                    ret = true;
                    goto END;
                }
                else
                {
                    break;
                }
            }
            //  retrieve the key and the length of the value
            string key = line.substr(leftangle + 1, colon - leftangle - 1);
            //  turn the key into lowercase
            transform(key.begin(), key.end(), key.begin(), ::tolower);
            //  the end of the record

            int len = stoi(line.substr(colon + 1, rightangle - colon - 1));
            //  there are chances that the length is 0
            if (len > 0)
            {
                string value = line.substr(rightangle + 1, len);
                // cout << key << ":" << value << endl;
                #ifdef _FILE_TEST_
                string nv;
                gbk2utf8(value, nv);
                nv = trim(nv);
                // if ( key == "freq" ) {
                //     nv.insert(nv.length()-3, ".");
                // }
                // fields[key] = nv;
                value = nv;
                #endif
                value = trim(value);
                if ( key == "time_on" && value.length() <6 ) {
                    value += "00";
                }
                fields[key] = value;
                // cout << key << "=" << nv << endl;
            } else {
                // cout << "len=0:" << key << endl;
            }
            //  move to the next field
            last = rightangle + len + 1;
        }
    }
END:
    return ret;
}

/**
 * @brief save the record to a file
 * @param out the output stream
 * @return true if successful
 * The format of the record is:
 * <key1:length1>value1<key2:length2>value2<key3:length3>value3<eor>
 * The least fields are to be saved first, in the order of least_keys.
 * The rest fileds are to be saved in the order of the alphabet order of the filed.
 */
bool adif::Record::save(ostream &out) const
{
    bool ret = false;

    if (iscomplete())
    {
        for (auto key : least_keys)
        {
            out << "<" << key << ":" << fields.at(key).length() << ">" << fields.at(key);
        }
        for (auto it = fields.begin(); it != fields.end(); it++)
        {
            if (find(least_keys, least_keys + sizeof(least_keys) / sizeof(least_keys[0]), it->first) == least_keys + sizeof(least_keys) / sizeof(least_keys[0]))
            {
                out << "<" << it->first << ":" << it->second.length() << ">" << it->second;
            }
        }
        out << "<eor>" << endl;
        ret = true;
    }
    return ret;
}

/**
 * @brief convert the record to a string
 * @return the string
 * The format of the record is:
 *
 * The least fields are to be saved first, in the order of least_keys.
 * The rest fileds are to be saved in the order of the alphabet order of the filed.
 */
string adif::Record::to_string() const
{
    string ret;
    if (iscomplete())
    {
        for (auto key : least_keys)
        {
            if (ret.empty())
            {
                ret = fields.at(key);
            }
            else
            {
                ret += "," + fields.at(key);
            }
        }
        for (auto it = fields.begin(); it != fields.end(); it++) {
            if (find(least_keys, least_keys + sizeof(least_keys) / sizeof(least_keys[0]), it->first)
             == least_keys + sizeof(least_keys) / sizeof(least_keys[0])) {
                ret += "," + it->first + ":" + it->second;
            }
        }
        // ret += "<eor>";
    }
    return ret;
}

std::time_t adif::Record::time_stamp() const
{
    // setenv("TZ", "/usr/share/zoneinfo/Europe/London", 1);
    std::tm t{};
    std::istringstream ss(date_time());
    ss >> std::get_time(&t, "%Y%m%d%H%M%S");
    if (ss.fail())
    {
        throw std::runtime_error{"failed to parse time string"};
    }
    return mktime(&t)+28800;    //  8 hours to UTC
}


set<string> adif::Record::enum_fields(void) const
{
    set<string> ret;
    for (auto it = fields.begin(); it != fields.end(); it++)
    {
        ret.insert(it->first);
    }
    return ret;
}

/**
 * @brief get the least key string
 * @return the least key string
 */
string &adif::Record::least_key_str(void)
{
    static string ret;
    if (ret.empty())
    {
        for (int i = 0; i < sizeof(least_keys) / sizeof(least_keys[0]); i++)
        {
            ret += least_keys[i]+",";
        }
    }
    return ret;
}

string adif::Record::export_csv(set<string> keys) const
{
    string ret;
    for (auto key : least_keys)
    {
        if (ret.empty())
        {
            ret = "\""+fields.at(key)+"\"";
        }
        else
        {
            ret += ",\"" + fields.at(key)+"\"";
        }
    }
    for ( auto k : keys ) {
        if (find(least_keys, least_keys + sizeof(least_keys) / sizeof(least_keys[0]), k) == least_keys + sizeof(least_keys) / sizeof(least_keys[0]))
        {
            if ( fields.count(k) > 0 ) {
                ret += ",\"" + fields.at(k)+"\"";
            } else {
                ret += ",";
            }
        }
    }
    return ret;

}

//  ------------------Private------------------

/**
 * @brief check if the record is complete
 * @return true if the record is complete
 * A record is complete if it has all the fields in least_keys.
 */
bool adif::Record::iscomplete() const
{
    bool ret = true;
    for (int i = 0; i < sizeof(least_keys) / sizeof(least_keys[0]); i++)
    {
        if (fields.count(least_keys[i]) == 0)
        {
            ret = false;
            break;
        }
    }
    return ret;
}

void adif::Record::merge(const Record &that)
{
    for (auto it = that.fields.begin(); it != that.fields.end(); it++)
    {
        if (fields.count(it->first) == 0)
        {
            fields[it->first] = it->second;
        }
    }
}

//  ------------------Test------------------

#ifdef _RECORD_TEST_
int main()
{
    adif::Record r;
    r.load(cin);
    // r.save(cout);
    cout << r.to_string() << endl;
    cout << r.time_stamp() << endl;
    return 0;
}
#endif