import re
from adif_config import get_adif_soup, save_cpp_header

def generate_award_map():
    """
    AWARD 
    (import-only) 
    """
    soup = get_adif_soup()
    if not soup: return

    table = soup.find('table', {'id': 'Enumeration_Award'})
    if not table:
        print("[-] Enumeration_Award not found")
        return

    map_entries = [] 
    for row in table.find_all('tr'):
        cols = row.find_all('td')
        if not cols: 
            continue
        
        cell = cols[0]
        raw_text = cell.get_text(" ", strip=True)
        award_name = re.sub(r'\(.*?\)', '', raw_text).strip()
        
        is_import_only = "import-only" in raw_text.lower()
        
        map_entries.append(f'    {{"{award_name}", {str(is_import_only).lower()}}},')

    cpp_content = (
        "/**\n"
        " * @brief ADIF Award Enumeration (Import-only markers included)\n"
        " * Key: Award Name, Value: true if the award is import-only.\n"
        " */\n\n"
        "using AwardMap = std::map<std::string, bool, CaseInsensitiveLess>;\n\n"
        "inline const AwardMap AWARD_MAP = {\n" 
        + "\n".join(map_entries) +
        "\n};"
    )

    save_cpp_header("award_map.h", cpp_content)

if __name__ == "__main__":
    generate_award_map()