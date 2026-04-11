import re
from adif_config import get_adif_soup, save_cpp_header

def generate_qso_download_status_map(): 
    soup = get_adif_soup()
    if not soup: return

    table = soup.find('table', {'id': 'Enumeration_QSO_Download_Status'})
    if not table:
        print("[-] Enumeration_QSO_Download_Status not found")
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
        " * ADIF QSO Download Status Enumeration\n"
        " */\n"
        "using QsoDownloadStatusMap = std::map<std::string, std::string>;\n\n"
        "static const QsoDownloadStatusMap QSO_DOWNLOAD_STATUS_MAP = {\n" 
        + "\n".join(map_entries) +
        "\n};"
    )

    save_cpp_header("qso_download_status_map.h", cpp_content)

if __name__ == "__main__":
    generate_qso_download_status_map()