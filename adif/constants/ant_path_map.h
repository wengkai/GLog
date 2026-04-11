#include <map>
#include <string>

using AntPathMap = std::map<std::string, std::string>;

static const AntPathMap ANT_PATH_MAP = {
    {"G", "grayline"},
    {"O", "other"},
    {"S", "short path"},
    {"L", "long path"},
};