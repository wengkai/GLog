import re
from adif_config import get_adif_soup, save_cpp_header

def generate_band_map():
    soup = get_adif_soup()
    if not soup: return

    table = soup.find('table', {'id': 'Enumeration_Band'})
    if not table:
        print("[-] Enumeration_Band not found")
        return

    map_entries = [] 
    for row in table.find_all('tr'):
        cols = row.find_all('td')
        if not cols: continue
        
        name = cols[0].get_text(strip=True).upper()
        low  = re.sub(r'[^\d.]', '', cols[1].get_text(strip=True))
        high = re.sub(r'[^\d.]', '', cols[2].get_text(strip=True))
        
        low = low if low else "0.0"
        high = high if high else "0.0"
        
        map_entries.append(f'    {{"{name}", {{"{low}", "{high}"}}}},')

    cpp_content = (
        "struct BandRange {\n"
        "    std::string lower_mhz;\n"
        "    std::string upper_mhz;\n"
        "};\n\n"
        "using BandMap = std::map<std::string, BandRange, std::less<>>;\n\n"
        "static const BandMap BAND_MAP = {\n" 
        + "\n".join(map_entries) +
        "\n};"
    )

    save_cpp_header("band_map.h", cpp_content)

if __name__ == "__main__":
    generate_band_map()