import re
from adif_config import get_adif_soup, save_cpp_header

def generate_morse_key_map(): 
    soup = get_adif_soup()
    if not soup: return

    table = soup.find('table', {'id': 'Enumeration_Morse_Key_Type'})
    if not table:
        print("[-] Enumeration_Morse_Key_Type not found")
        return

    map_entries = [] 

    for row in table.find_all('tr'):
        cols = row.find_all('td')
        if not cols or len(cols) < 4: continue
        
        abbr = cols[0].get_text(strip=True).replace('\xa0', ' ')
        meaning = cols[1].get_text(strip=True).replace('\xa0', ' ')
        composition = cols[3].get_text(strip=True).replace('\xa0', ' ')
        
        if not abbr: continue

        cpp_line = f'    {{"{abbr}", {{"{meaning}", "{composition}"}}}},'
        map_entries.append(cpp_line)

    cpp_content = (
        "struct MorseKeyInfo {\n"
        "    std::string meaning;     \n"
        "    std::string composition; \n"
        "};\n\n"
        "using MorseKeyMap = std::map<std::string, MorseKeyInfo, CaseInsensitiveLess>;\n\n"
        "inline const MorseKeyMap MORSE_KEY_MAP = {\n" 
        + "\n".join(map_entries) +
        "\n};"
    )

    save_cpp_header("morse_key_map.h", cpp_content)

if __name__ == "__main__":
    generate_morse_key_map()