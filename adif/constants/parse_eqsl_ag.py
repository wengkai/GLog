import re
from adif_config import get_adif_soup, save_cpp_header

def generate_eqsl_ag_map():
    soup = get_adif_soup()
    if not soup:
        return

    table = soup.find('table', {'id': 'Enumeration_EQSL_AG'})
    if not table:
        print("[-] Enumeration_EQSL_AG table not found")
        return

    map_entries = []

    for row in table.find_all('tr'):
        cols = row.find_all('td')
        if len(cols) < 2:
            continue

        try:
            status = cols[0].get_text(strip=True).replace('\xa0', '')
            if not status:
                continue

            description = cols[1].get_text(strip=True).replace('"', '\\"')
            if not description:
                continue
            
            description = ' '.join(description.split())
            map_entries.append((status, description))
        except (ValueError, IndexError):
            continue

    map_entries.sort(key=lambda x: x[0])

    formatted_lines = [
        f'        {{"{status}", "{description}"}},'
        for status, description in map_entries
    ]

    cpp_content = """
using EQSL_AG_Container = std::map<std::string, std::string, CaseInsensitiveLess>;

inline const EQSL_AG_Container EQSL_AG_MAP = {
""" + "\n".join(formatted_lines) + "\n};"

    require = """#include <string>
#include <map>
"""

    save_cpp_header("eqsl_ag_map.h", cpp_content, require)

if __name__ == "__main__":
    generate_eqsl_ag_map()