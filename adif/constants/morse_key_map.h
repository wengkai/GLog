#ifndef MORSE_KEY_MAP_H_GENERATED_H
#define MORSE_KEY_MAP_H_GENERATED_H

// clang-format off

#include "CaseInsensitiveLess.h"


namespace ADIF {
struct MorseKeyInfo {
    std::string meaning;     
    std::string composition; 
};

using MorseKeyMap = std::map<std::string, MorseKeyInfo, CaseInsensitiveLess>;

inline const MorseKeyMap MORSE_KEY_MAP = {
    {"SK", {"Straight Key", "a human makes the dits and dahs and builds characters"}},
    {"SS", {"Sideswiper", "a human makes the dits and dahs and builds characters"}},
    {"BUG", {"Mechanical semi-automatic keyer or Bug", "a machine makes the dits and a human makes the dahs and builds characters."}},
    {"FAB", {"Mechanical fully-automatic keyer or Bug", "a machine makes the dits and the dahs and a human builds characters."}},
    {"SP", {"Single Paddle", "a machine makes the dits and the dahs and a human builds the characters."}},
    {"DP", {"Dual Paddle", "a machine makes the dits and the dahs and a human builds the characters."}},
    {"CPU", {"Computer Driven", "a machine makes the dits and dahs to build the characters."}},
};
} // namespace ADIF

// clang-format on

#endif // MORSE_KEY_MAP_H_GENERATED_H
