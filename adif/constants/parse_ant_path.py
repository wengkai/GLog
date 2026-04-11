import re
from adif_config import get_adif_soup, save_cpp_header

def generate_ant_path_map():
    """
    ANT_PATH
    """
    soup = get_adif_soup()
    if not soup:
        return

    table = soup.find('table', {'id': 'Enumeration_Ant_Path'})
    if not table:
        print("[-] Enumeration_Ant_Path not found")
        return

    map_entries = []  
    for row in table.find_all('tr'):
        cols = row.find_all('td')
        if not cols: 
            continue
        
        abbr = cols[0].get_text(strip=True).replace('\xa0', ' ')
        meaning = cols[1].get_text(strip=True).replace('\xa0', ' ')
        
        map_entries.append(f'    {{"{abbr}", "{meaning}"}},')

    cpp_content = (
        "using AntPathMap = std::map<std::string, std::string>;\n\n"
        "static const AntPathMap ANT_PATH_MAP = {\n"  
        + "\n".join(map_entries) +
        "\n};"
    )

    save_cpp_header("ant_path_map.h", cpp_content)

if __name__ == "__main__":
    generate_ant_path_map()