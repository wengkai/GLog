import re
from adif_config import get_adif_soup, save_cpp_header

def _process_mode_cell(cell):
    import_only_link = cell.find('a', string=re.compile(r'import-only', re.I))
    is_imported = import_only_link is not None
    
    text = cell.get_text().replace('\xa0', ' ')
    if is_imported:
        text = text.replace("(import-only)", "")
    
    return text.strip(), is_imported

def _parse_submodes(cell):
    raw_text = cell.get_text(separator=' ', strip=True).replace('\xa0', ' ')
    items = re.split(r'[,\r\n]+', raw_text)
    return [s.strip() for s in items if s.strip()]

def generate_mode_map(): 
    soup = get_adif_soup()
    if not soup: return

    table = soup.find('table', {'id': 'Enumeration_Mode'})
    if not table:
        print("[-] Enumeration_Mode not found")
        return

    map_entries = [] 
    for row in table.find_all('tr'):
        cols = row.find_all('td')
        if not cols: continue
        
        mode_name, is_imported = _process_mode_cell(cols[0])
        if not mode_name: continue
        
        submodes = _parse_submodes(cols[1])
        submodes_formatted = ", ".join([f'"{s}"' for s in submodes])
        
        cpp_line = f'    {{"{mode_name}", {{{str(is_imported).lower()}, {{{submodes_formatted}}}}}}},'
        map_entries.append(cpp_line)

    cpp_content = (
        "#ifndef MODE_MAP_T\n"
        "#define MODE_MAP_T\n"
        "struct ModeInfo {\n"
        "    bool import_only;               \n"
        "    std::vector<std::string> submodes; \n"
        "};\n\n"
        "using ModeMap = std::map<std::string, ModeInfo>;\n\n"
        "#endif\n"
        "static const ModeMap MODE_MAP = {\n" 
        + "\n".join(map_entries) +
        "\n};"
    )

    save_cpp_header("mode_map.h", cpp_content)

if __name__ == "__main__":
    generate_mode_map()