import re
from adif_config import get_adif_soup, save_cpp_header

def generate_dxcc_map(): 
    soup = get_adif_soup()
    if not soup: 
        return

    table = soup.find('table', {'id': 'Enumeration_DXCC_Entity_Code'})
    if not table:
        print("[-] Enumeration_DXCC_Entity_Code not found")
        return

    map_entries = [] 
    
    for row in table.find_all('tr'):
        cols = row.find_all('td')
        if not cols: 
            continue
        
        try:
            code_raw = cols[0].get_text(strip=True).replace('\xa0', '')
            if not code_raw: continue
            code = int(code_raw)
            
            name = cols[1].get_text(strip=True).replace('\xa0', ' ').replace('"', '\\"')
            is_deleted = "true" if "Y" in cols[2].get_text(strip=True).upper() else "false"
            
            map_entries.append((code, name, is_deleted))
        except (ValueError, IndexError):
            continue

    map_entries.sort(key=lambda x: x[0])

    formatted_lines = [f'        {{{c}, {{"{n}", {d}}}}},' for c, n, d in map_entries]

    cpp_content = """

struct DXCCEntity {
    std::string name;
    bool is_deleted;
};

/**
 * @brief DXCC Entity Container
 */
class DXCCContainer : public std::vector<std::pair<int, DXCCEntity>> {
public:
    using std::vector<std::pair<int, DXCCEntity>>::vector; 

    const_iterator find(int code) const {
        auto it = std::lower_bound(begin(), end(), code, 
            [](const std::pair<int, DXCCEntity>& element, int val) {
                return element.first < val;
            });
        return (it != end() && it->first == code) ? it : end();
    }
    
    bool contains(int code) const {
        return find(code) != end();
    }
};

static const DXCCContainer DXCC_MAP = {
""" + "\n".join(formatted_lines) + "\n};" 
    
    require = """#include <vector>
#include <algorithm>
"""

    save_cpp_header("dxcc_map.h", cpp_content, require)

if __name__ == "__main__":
    generate_dxcc_map()