import re
from adif_config import get_adif_soup, save_cpp_header

def generate_qsl_via_map(): 
    soup = get_adif_soup()
    if not soup: return

    table = soup.find('table', {'id': 'Enumeration_QSL_Via'})
    if not table:
        print("[-] Enumeration_QSL_Via not found")
        return

    map_entries = [] 
    for row in table.find_all('tr'):
        cols = row.find_all('td')
        if not cols: continue
        
        via_raw = cols[0].get_text(separator=' ', strip=True).replace('\xa0', ' ')
        import_link = cols[0].find('a', string=re.compile(r'import-only', re.I))
        is_imported = import_link is not None
        
        via_code = re.sub(r'\(import-only\)', '', via_raw, flags=re.I).strip()
        
        desc_raw = cols[1].get_text(separator=' ', strip=True).replace('\xa0', ' ')
        description = re.sub(r'\(import-only\)', '', desc_raw, flags=re.I).strip()
        
        if not via_code: continue

        cpp_line = f'    {{"{via_code}", {{{str(is_imported).lower()}, "{description}"}}}},'
        map_entries.append(cpp_line)

    cpp_content = (
        "struct QslViaInfo {\n"
        "    bool import_only;\n"
        "    std::string description;\n"
        "};\n\n"
        "using QslViaMap = std::map<std::string, QslViaInfo>;\n\n"
        "static const QslViaMap QSL_VIA_MAP = {\n" 
        + "\n".join(map_entries) +
        "\n};"
    )

    save_cpp_header("qsl_via_map.h", cpp_content)

if __name__ == "__main__":
    generate_qsl_via_map()