from adif_config import get_adif_soup, save_cpp_header

def generate_credit_map(): 
    soup = get_adif_soup()
    if not soup:
        return

    table = soup.find('table', {'id': 'Enumeration_Credit'})
    if not table:
        print("[-] Enumeration_Credit not found")
        return

    map_entries = [] 
    for row in table.find_all('tr'):
        cols = row.find_all('td')
        if not cols or cols[0].get_text(strip=True) == "Credit For":
            continue
            
        credit_id = cols[0].get_text(strip=True)
        sponsor = cols[1].get_text(strip=True)
        award = cols[2].get_text(strip=True)
        facet = cols[3].get_text(strip=True)
        
        cpp_line = f'    {{"{credit_id}", {{"{sponsor}", "{award}", "{facet}"}}}},'
        map_entries.append(cpp_line)

    cpp_content = (
        "struct CreditInfo {\n"
        "    std::string sponsor; \n"
        "    std::string award;   \n"
        "    std::string facet;   \n"
        "};\n\n"
        "using CreditMap = std::map<std::string, CreditInfo>;\n\n"
        "static const CreditMap CREDIT_MAP = {\n" 
        + "\n".join(map_entries) +
        "\n};"
    )

    save_cpp_header("credit_map.h", cpp_content)

if __name__ == "__main__":
    generate_credit_map()