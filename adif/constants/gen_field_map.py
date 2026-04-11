import re
from adif_config import get_adif_soup, save_cpp_header


TYPE_MAP = {
    "String": "AdifString",
    "MultilineString": "AdifMultilineString",
    "IntlString": "AdifIntlString",
    "IntlMultilineString": "AdifIntlMultilineString",
    "Date": "AdifDate",
    "Time": "AdifTime",
    "Number": "AdifNumber",
    "Integer": "AdifInteger",
    "PositiveInteger": "AdifPositiveInteger",
    "Boolean": "AdifBoolean",
    "Location": "AdifLocation",
    "GridSquare": "AdifGridSquare",
    "GridSquareExt": "AdifGridSquareExt",
    "GridSquareList": "AdifGridSquareList",
    "POTARef": "AdifPOTARef",
    "POTARefList": "AdifPOTARefList",
    "SOTARef": "AdifSOTARef",
    "WWFFRef": "AdifWWFFRef"
}

def generate_field_map():
    soup = get_adif_soup()
    if not soup:
        return

    table = soup.find('table', {'id': 'Field_QSO'})
    if not table:
        print("[-] Field_QSO table not found")
        return

    field_entries = []

    for row in table.find_all('tr'):
        cols = row.find_all('td')
        if len(cols) < 2:
            continue

        field_name = cols[0].get_text(strip=True).lower()
        
        type_link = cols[1].find('a')
        type_raw = type_link.get_text(strip=True) if type_link else cols[1].get_text(strip=True)
        
        cpp_class = TYPE_MAP.get(type_raw)
        
        if cpp_class:
            field_entries.append((field_name, cpp_class))

    field_entries.sort(key=lambda x: x[0])

    formatted_lines = [f'    NEW_FIELD_ENTRY({name}, {classname}),' for name, classname in field_entries]

    cpp_content = """/**
 * @file adif_field_map.h
 */

#ifndef ADIF_FIELD_MAP_H
#define ADIF_FIELD_MAP_H

static const std::map<std::string, AdifFactoryFunc> ADIF_FIELD_FACTORY = {
""" + "\n".join(formatted_lines) + """
};

#endif // ADIF_FIELD_MAP_H
"""

    save_cpp_header("adif_field_map.h", cpp_content)

if __name__ == "__main__":
    generate_field_map()