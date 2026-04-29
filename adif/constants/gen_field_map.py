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
    "WWFFRef": "AdifWWFFRef",
    "CreditList": "AdifCreditList",
    "IOTARefNo": "AdifIOTARefNo",
}

ENUM_MAP = {
    "Ant Path": "AdifAntPath",
    "ARRL Section": "AdifArrlSection",
    "Band": "AdifBandRx",
    "EQSL_AG": "AdifEqslAg",
    "QSO Upload Status": "AdifQsoUploadStatus",
    "Continent": "AdifContinent",
    "QSL Rcvd": "AdifQslRcvd",
    "QSL Sent": "AdifQslSent",
    "DXCC Entity Code": "AdifDxcc",
    "Morse Key Type": "AdifMorseKey",
    "Propagation Mode": "AdifPropagation",
    "QSO Download Status": "AdifQsoDownloadStatus",
    "QSL Via": "AdifQslVia",
    "QSO Complete": "AdifQsoComplete",
    "Region": "AdifRegion",
}

SPECIAL_MAP = {
    "mode",
    "submode",
    "band",
    "freq",
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
        if len(cols) < 3:
            continue

        field_name = cols[0].get_text(strip=True).lower()

        if field_name in SPECIAL_MAP:
            continue
        
        type_link = cols[1].find('a')
        type_raw = type_link.get_text(strip=True) if type_link else cols[1].get_text(strip=True)

        if type_raw == 'Enumeration':
            enum_link = cols[2].find('a')
            enum_raw = enum_link.get_text(strip=True) if enum_link else cols[2].get_text(strip=True)
            enum_raw = ' '.join(enum_raw.split())
            try_str = f'Enum {field_name} -> {enum_raw}'
            cpp_class = ENUM_MAP.get(enum_raw)
        else:
            try_str = f'Type {field_name} -> {type_raw}'
            cpp_class = TYPE_MAP.get(type_raw)
        
        if cpp_class:
            field_entries.append((field_name, cpp_class))
            print(f'[+]Done: {try_str} -> {cpp_class}')
        else:
            print(f'[!]Skip: {try_str} -> ?')

    field_entries.sort(key=lambda x: x[0])

    formatted_lines = [f'    NEW_FIELD_ENTRY({name}, {classname}),' for name, classname in field_entries]

    cpp_content = """
static const std::unordered_map<std::string, AdifFactoryFunc> ADIF_FIELD_FACTORY = {
""" + "\n".join(formatted_lines) + """
};
"""

    require = """/**
 * @file adif_field_map.h
 */

#include <unordered_map>
"""

    save_cpp_header("adif_field_map.h", cpp_content, require)

if __name__ == "__main__":
    generate_field_map()