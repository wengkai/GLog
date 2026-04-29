import re
from adif_config import get_adif_soup, save_cpp_header

def generate_region_map(): 
    soup = get_adif_soup()
    if not soup: return

    table = soup.find('table', {'id': 'Enumeration_Region'})
    if not table:
        print("[-] Enumeration_Region not found")
        return

    map_entries = [] 
    for row in table.find_all('tr'):
        cols = row.find_all('td')
        if not cols: continue
        
        region_code = cols[0].get_text(strip=True).replace('\xa0', ' ')
        
        dxcc_raw = cols[1].get_text(strip=True).replace('\xa0', '')
        dxcc_code = dxcc_raw if dxcc_raw.isdigit() else "0"
        
        region_name = cols[2].get_text(strip=True).replace('\xa0', ' ')
        prefix      = cols[3].get_text(strip=True).replace('\xa0', ' ')
        
        apps = [a.get_text(strip=True) for a in cols[4].find_all('a')]
        if not apps: 
            apps = [cols[4].get_text(strip=True).replace('\xa0', ' ')]
        apps_str = ",".join(filter(None, apps))

        start_date = cols[5].get_text(strip=True).replace('\xa0', '')
        end_date   = cols[6].get_text(strip=True).replace('\xa0', '')

        if not region_code: continue

        cpp_line = (
            f'    {{"{region_code}", {{ {dxcc_code}, "{region_name}", "{prefix}", '
            f'"{apps_str}", "{start_date}", "{end_date}" }} }},'
        )
        map_entries.append(cpp_line)

    cpp_content = (
        "struct RegionEntry {\n"
        "    int dxcc_entity_code;\n"
        "    std::string name;\n"
        "    std::string prefix;\n"
        "    std::string applicability; // CQ,WAE\n"
        "    std::string start_date;    // YYYY-MM-DD\n"
        "    std::string end_date;      // YYYY-MM-DD\n"
        "};\n\n"
        "/**\n"
        " * @brief ADIF Region Multimap\n"
        " */\n"
        "using RegionMap = std::multimap<std::string, RegionEntry, CaseInsensitiveLess>;\n\n"
        "inline const RegionMap REGION_MAP = {\n" 
        + "\n".join(map_entries) +
        "\n};"
    )

    save_cpp_header("region_map.h", cpp_content)

if __name__ == "__main__":
    generate_region_map()