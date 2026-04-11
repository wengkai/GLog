import re
from adif_config import get_adif_soup, save_cpp_header

def _process_qsl_status_cell(cell):
    import_only_link = cell.find('a', string=re.compile(r'import-only', re.I))
    is_imported = import_only_link is not None
    
    raw_text = cell.get_text(separator=' ', strip=True).replace('\xa0', ' ')
    clean_code = re.sub(r'\(import-only\)', '', raw_text, flags=re.I).strip()
    
    return clean_code, is_imported

def _parse_qsl_description(cell):
    if not cell:
        return ""
    
    items = [li.get_text(strip=True).replace('\xa0', ' ') for li in cell.find_all('li')]
    if items:
        return " ; ".join(items)
    
    return cell.get_text(strip=True).replace('\xa0', ' ')

def generate_qsl_rcvd_map(): 
    soup = get_adif_soup()
    if not soup: return

    table = soup.find('table', {'id': 'Enumeration_QSL_Rcvd'})
    if not table:
        print("[-] Enumeration_QSL_Rcvd not found")
        return

    map_entries = [] 
    for row in table.find_all('tr'):
        cols = row.find_all('td')
        if not cols: continue
        
        status_code, is_imported = _process_qsl_status_cell(cols[0])
        if not status_code: continue
        
        meaning = cols[1].get_text(strip=True).replace('\xa0', ' ')
        description = _parse_qsl_description(cols[2])
        
        cpp_line = f'    {{"{status_code}", {{{str(is_imported).lower()}, "{meaning}", "{description}"}}}},'
        map_entries.append(cpp_line)

    cpp_content = (
        "#ifndef QSL_STATUS_INFO_DEFINED\n"
        "#define QSL_STATUS_INFO_DEFINED\n"
        "struct QslStatusInfo {\n"
        "    bool import_only;\n"
        "    std::string meaning;\n"
        "    std::string description;\n"
        "};\n"
        "#endif\n\n"
        "using QslRcvdMap = std::map<std::string, QslStatusInfo>;\n\n"
        "static const QslRcvdMap QSL_RCVD_MAP = {\n" 
        + "\n".join(map_entries) +
        "\n};"
    )

    save_cpp_header("qsl_rcvd_map.h", cpp_content)

if __name__ == "__main__":
    generate_qsl_rcvd_map()