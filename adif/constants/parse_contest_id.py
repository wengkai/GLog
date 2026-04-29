from adif_config import get_adif_soup, save_cpp_header

def generate_contest_id_map():
    soup = get_adif_soup()
    if not soup: return

    table = soup.find('table', {'id': 'Enumeration_Contest_ID'})
    if not table:
        print("[-] Enumeration_Contest_ID not found")
        return

    map_entries = [] 
    for row in table.find_all('tr')[1:]:
        cols = row.find_all('td')
        if len(cols) < 2: continue

        contest_id = cols[0].get_text(strip=True).replace('\xa0', ' ')
        
        desc_parts = list(cols[1].stripped_strings)
        description = " ".join(desc_parts)
        
        description = description.replace('"', '\\"')

        map_entries.append(f'    {{"{contest_id}", "{description}"}},')

    cpp_content = (
        "/**\n"
        " * @brief ADIF Contest-ID Enumeration\n"
        " * Maps Contest-ID codes to their official descriptions.\n"
        " */\n\n"
        "using ContestIdMap = std::map<std::string, std::string, CaseInsensitiveLess>;\n\n"
        "inline const ContestIdMap CONTEST_ID_MAP = {\n" 
        + "\n".join(map_entries) +
        "\n};"
    )

    save_cpp_header("contest_id_map.h", cpp_content)

if __name__ == "__main__":
    generate_contest_id_map()