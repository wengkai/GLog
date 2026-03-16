import re
import requests
from bs4 import BeautifulSoup

def get_html_from_url(url):
    headers = {
        "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
        "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8",
        "Accept-Language": "en-US,en;q=0.5",
        "Referer": "https://adif.org.uk/",
        "DNT": "1" 
    }
    try:
        response = requests.get(url, headers=headers, timeout=10)
        response.raise_for_status()
        response.encoding = response.apparent_encoding
        return response.text
    except requests.exceptions.RequestException as e:
        print(f"Error fetching URL: {e}")
        return None

def process_cell_content(cell):
    raw_html = cell.decode_contents()
    pattern = r'\s*\(\s*<a[^>]*>import-only<\/a>\s*\)\s*'
    is_imported = bool(re.search(pattern, raw_html))
    clean_text = re.sub(pattern, '', raw_html)
    clean_text = BeautifulSoup(clean_text, 'html.parser').get_text(strip=True)
    return clean_text, is_imported

def parse_submodes(raw_text):
    cleaned_text = raw_text.replace('\xa0', ' ').replace('&nbsp;', ' ')
    
    items = re.split(r'[,\r\n]+', cleaned_text)
    
    return [item.strip() for item in items if item.strip()]

def parse_and_generate(url, output_file="mode_map.h"):
    html_content = get_html_from_url(url)
    if not html_content: return

    soup = BeautifulSoup(html_content, 'html.parser')
    table = soup.find('table', {'id': 'Enumeration_Mode'})
    if not table:
        print("Table 'Enumeration_Mode' not found.")
        return

    lines = []
    for row in table.find_all('tr')[1:]:
        cols = row.find_all('td')
        if len(cols) < 2: continue
        
        name, is_imported = process_cell_content(cols[0])
        
        raw_sub = cols[1].get_text()
        submodes = parse_submodes(raw_sub)
        
        submodes_str = ", ".join([f'"{s}"' for s in submodes])
        line = f'    {{"{name}", {{{str(is_imported).lower()}, {{{submodes_str}}}}}}},'
        lines.append(line)
        
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("static const ModeMap mode_map = {\n")
        f.write("\n".join(lines))
        f.write("\n};")
    print(f"File '{output_file}' generated.")

parse_and_generate("https://adif.org.uk/316/ADIF_316.htm")