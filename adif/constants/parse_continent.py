from adif_config import get_adif_soup, save_cpp_header

def generate_continent_map(): 
    soup = get_adif_soup()
    if not soup:
        return

    table = soup.find('table', {'id': 'Enumeration_Continent'})
    if not table:
        print("[-] Enumeration_Continent not found")
        return

    map_entries = [] 
    for row in table.find_all('tr'):
        cols = row.find_all('td')
        if not cols or len(cols) < 2:
            continue
            
        abbr = cols[0].get_text(strip=True)
        full_name = cols[1].get_text(strip=True)
        
        if abbr and full_name:
            cpp_line = f'    {{"{abbr}", "{full_name}"}},'
            map_entries.append(cpp_line)

    cpp_content = (
        "// ADIF Continent Enumeration Mapping\n"
        "using ContinentMap = std::map<std::string, std::string>;\n\n"
        "static const ContinentMap CONTINENT_MAP = {\n" 
        + "\n".join(map_entries) +
        "\n};"
    )

    save_cpp_header("continent_map.h", cpp_content)

if __name__ == "__main__":
    generate_continent_map()