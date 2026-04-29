import re
from adif_config import get_adif_soup, save_cpp_header

def generate_submode_map(): 
    soup = get_adif_soup()
    if not soup: return

    table = soup.find('table', {'id': 'Enumeration_Submode'})
    if not table:
        print("[-] Enumeration_Submode not found")
        return

    map_entries = [] 
    for row in table.find_all('tr'):
        cols = row.find_all('td')
        if not cols: continue
        
        submode = cols[0].get_text(strip=True).replace('\xa0', ' ')
        
        parent_mode = cols[1].get_text(strip=True).replace('\xa0', ' ')
        
        description = cols[2].get_text(" ", strip=True).replace('\xa0', ' ').replace('"', '\\"')

        if not submode: continue

        cpp_line = f'    {{"{submode}", {{ "{parent_mode}", "{description}" }} }},'
        map_entries.append(cpp_line)

    cpp_content = (
        "struct SubmodeEntry {\n"
        "    std::string parent_mode; // (MODE)\n"
        "    std::string description; \n"
        "};\n\n"
        "using SubmodeMap = std::map<std::string, SubmodeEntry, CaseInsensitiveLess>;\n\n"
        "/**\n"
        " * @brief ADIF Submode to Parent Mode mapping\n"
        " */\n"
        "inline const SubmodeMap SUBMODE_MAP = {\n" 
        + "\n".join(map_entries) +
        "\n};"
    )

    save_cpp_header("submode_map.h", cpp_content)

if __name__ == "__main__":
    generate_submode_map()