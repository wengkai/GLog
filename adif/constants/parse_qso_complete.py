import re
from adif_config import get_adif_soup, save_cpp_header

def generate_qso_complete_map(): 
    soup = get_adif_soup()
    if not soup: return

    table = soup.find('table', {'id': 'Enumeration_QSO_Complete'})
    if not table:
        print("[-] Enumeration_QSO_Complete not found")
        return

    map_entries = [] 
    for row in table.find_all('tr'):
        cols = row.find_all('td')
        if not cols: continue
        
        abbr = cols[0].get_text(strip=True).replace('\xa0', ' ')
        meaning = cols[1].get_text(strip=True).replace('\xa0', ' ')
        
        if not abbr: continue

        cpp_line = f'    {{"{abbr}", "{meaning}"}},'
        map_entries.append(cpp_line)

    cpp_content = (
        "/**\n"
        " * ADIF QSO Complete Enumeration\n"
        " */\n"
        "using QsoCompleteMap = std::map<std::string, std::string, CaseInsensitiveLess>;\n\n"
        "inline const QsoCompleteMap QSO_COMPLETE_MAP = {\n" 
        + "\n".join(map_entries) +
        "\n};"
    )

    save_cpp_header("qso_complete_map.h", cpp_content)

if __name__ == "__main__":
    generate_qso_complete_map()