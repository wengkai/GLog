import re
from adif_config import get_adif_soup, save_cpp_header

def generate_arrl_section_map():
    soup = get_adif_soup()
    if not soup: return

    table = soup.find('table', {'id': 'Enumeration_ARRL_Section'})
    if not table:
        print("[-] Enumeration_ARRL_Section not found")
        return

    map_entries = [] 
    for row in table.find_all('tr')[1:]:
        cols = row.find_all('td')
        if not cols: continue

        abbr = cols[0].get_text(strip=True)
        
        name_raw = cols[1].get_text(" ", strip=True)
        name = name_raw.split('(')[0].strip()
        
        dxcc_codes = [re.sub(r'\D', '', a.text) for a in cols[2].find_all('a')]
        if not dxcc_codes: 
             dxcc_codes = re.findall(r'\d+', cols[2].get_text())
        dxcc_str = ", ".join(dxcc_codes)

        start_date = cols[3].get_text(strip=True).replace('\xa0', '')
        end_date = cols[4].get_text(strip=True).replace('\xa0', '')

        line = (f'    {{"{abbr}", {{"{name}", {{{dxcc_str}}}, '
                f'"{start_date}", "{end_date}"}}}},')
        map_entries.append(line)

    cpp_content = (
        "struct ArrlSectionInfo {\n"
        "    std::string name;\n"
        "    std::vector<int> dxcc_entities;\n"
        "    std::string from_date;\n"
        "    std::string deleted_date;\n"
        "};\n\n"
        "using ArrlSectionMap = std::map<std::string, ArrlSectionInfo>;\n\n"
        "static const ArrlSectionMap ARRL_SECTION_MAP = {\n" 
        + "\n".join(map_entries) +
        "\n};"
    )

    save_cpp_header("arrl_section_map.h", cpp_content)

if __name__ == "__main__":
    generate_arrl_section_map()