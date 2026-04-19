import requests
import os
from bs4 import BeautifulSoup

ADIF_URL = "https://adif.org.uk/316/ADIF_316.htm"
HEADERS = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
    "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8",
    "Accept-Language": "en-US,en;q=0.5",
    "Referer": "https://adif.org.uk/",
    "DNT": "1" 
}

def get_adif_soup(url=ADIF_URL):
    try:
        response = requests.get(url, headers=HEADERS, timeout=10)
        response.raise_for_status()
        response.encoding = response.apparent_encoding
        return BeautifulSoup(response.text, 'html.parser')
    except Exception as e:
        print(f"[-] Failed to get html: {e}")
        return None

def save_cpp_header(filename, content, require = None):
    content = content.replace('\u00a0', ' ')
    guard_name = os.path.basename(filename).replace('.', '_').upper() + "_GENERATED_H"
    with open(filename, "w", encoding="utf-8") as f:
        f.write(f"#ifndef {guard_name}\n")
        f.write(f"#define {guard_name}\n\n")
        f.write("// clang-format off\n\n")
        if require is not None:
            f.write(require)
        f.write("\nnamespace ADIF {\n")
        f.write(content)
        f.write("\n} // namespace ADIF\n")
        f.write("\n// clang-format on\n\n")
        f.write(f"#endif // {guard_name}\n")
    print(f"[+] Success: {filename}")