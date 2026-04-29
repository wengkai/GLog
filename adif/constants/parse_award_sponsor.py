from adif_config import get_adif_soup, save_cpp_header

def generate_award_sponsor_map(): 
    soup = get_adif_soup()
    if not soup: return

    table = soup.find('table', {'id': 'Enumeration_Award_Sponsor'})
    if not table:
        print("[-] Enumeration_Award_Sponsor not found")
        return

    map_entries = [] 
    for row in table.find_all('tr'):
        cols = row.find_all('td')
        if not cols: continue
        
        prefix = cols[0].get_text(strip=True)
        org_name = cols[1].get_text(strip=True)
        
        cpp_line = f'    {{"{prefix}", "{org_name}"}},'
        map_entries.append(cpp_line)

    cpp_content = (
        "using AwardSponsorMap = std::map<std::string, std::string, CaseInsensitiveLess>;\n\n"
        "/**\n"
        " * @brief ADIF Award Sponsor Prefixes\n"
        " * Used for validating SPONSOR_PROGRAM_AWARD format.\n"
        " */\n"
        "inline const AwardSponsorMap AWARD_SPONSOR_MAP = {\n" 
        + "\n".join(map_entries) +
        "\n};"
    )

    save_cpp_header("award_sponsor_map.h", cpp_content)

if __name__ == "__main__":
    generate_award_sponsor_map()