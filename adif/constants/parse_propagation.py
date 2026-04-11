import re
from adif_config import get_adif_soup, save_cpp_header

def generate_propagation_map(): 
    soup = get_adif_soup()
    if not soup: return

    table = soup.find('table', {'id': 'Enumeration_Propagation_Mode'})
    if not table:
        print("[-] Enumeration_Propagation_Mode not found")
        return

    map_entries = [] 
    for row in table.find_all('tr'):
        cols = row.find_all('td')
        
        if not cols or len(cols) < 2:
            continue
        
        first_col_text = cols[0].get_text(strip=True)
        if "Enumeration" in first_col_text or "Mode" in first_col_text:
            continue
        
        prop_code = first_col_text.replace('\xa0', ' ')
        description = cols[1].get_text(strip=True).replace('\xa0', ' ')
        
        if not prop_code: continue

        cpp_line = f'    {{"{prop_code}", "{description}"}},'
        map_entries.append(cpp_line)

    cpp_content = (
        "/**\n"
        " * ADIF Propagation Modes\n"
        " */\n"
        "using PropagationMap = std::map<std::string, std::string>;\n\n"
        "static const PropagationMap PROPAGATION_MAP = {\n" 
        + "\n".join(map_entries) +
        "\n};"
    )

    save_cpp_header("propagation_map.h", cpp_content)

if __name__ == "__main__":
    generate_propagation_map()