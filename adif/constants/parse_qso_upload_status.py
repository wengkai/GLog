import re
from adif_config import get_adif_soup, save_cpp_header

def generate_qso_upload_status_map(): 
    soup = get_adif_soup()
    if not soup: return

    table = soup.find('table', {'id': 'Enumeration_QSO_Upload_Status'})
    if not table:
        print("[-] Enumeration_QSO_Upload_Status not found")
        return

    map_entries = [] 
    for row in table.find_all('tr'):
        cols = row.find_all('td')
        if not cols: continue
        
        status_code = cols[0].get_text(strip=True).replace('\xa0', ' ')
        description = cols[1].get_text(strip=True).replace('\xa0', ' ')
        
        if not status_code: continue

        cpp_line = f'    {{"{status_code}", "{description}"}},'
        map_entries.append(cpp_line)

    cpp_content = (
        "/**\n"
        " * ADIF QSO Upload Status Enumeration\n"
        " */\n"
        "using QsoUploadStatusMap = std::map<std::string, std::string>;\n\n"
        "static const QsoUploadStatusMap QSO_UPLOAD_STATUS_MAP = {\n" 
        + "\n".join(map_entries) +
        "\n};"
    )

    save_cpp_header("qso_upload_status_map.h", cpp_content)

if __name__ == "__main__":
    generate_qso_upload_status_map()