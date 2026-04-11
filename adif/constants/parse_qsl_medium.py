import re
from adif_config import get_adif_soup, save_cpp_header

def generate_qsl_medium_map(): 
    soup = get_adif_soup()
    if not soup: return

    table = soup.find('table', {'id': 'Enumeration_QSL_Medium'})
    if not table:
        print("[-] Enumeration_QSL_Medium not found")
        return

    map_entries = [] 
    for row in table.find_all('tr'):
        cols = row.find_all('td')
        if not cols: continue
        
        medium_code = cols[0].get_text(strip=True).replace('\xa0', ' ')
        description = cols[1].get_text(strip=True).replace('\xa0', ' ')
        
        if not medium_code: continue

        cpp_line = f'    {{"{medium_code}", "{description}"}},'
        map_entries.append(cpp_line)

    cpp_content = (
        "/**\n"
        " * ADIF QSL Medium Enumeration\n"
        " */\n"
        "using QslMediumMap = std::map<std::string, std::string>;\n\n"
        "static const QslMediumMap QSL_MEDIUM_MAP = {\n" 
        + "\n".join(map_entries) +
        "\n};"
    )

    save_cpp_header("qsl_medium_map.h", cpp_content)

if __name__ == "__main__":
    generate_qsl_medium_map()