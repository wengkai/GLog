#include <map>
#include <string>

// ADIF Continent Enumeration Mapping
using ContinentMap = std::map<std::string, std::string>;

static const ContinentMap CONTINENT_MAP = {
    {"NA", "North America"}, {"SA", "South America"}, {"EU", "Europe"},     {"AF", "Africa"},
    {"OC", "Oceania"},       {"AS", "Asia"},          {"AN", "Antarctica"},
};