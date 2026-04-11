import requests
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

def save_cpp_header(filename, content):
    content = content.replace('\u00a0', ' ')
    with open(filename, "w", encoding="utf-8") as f:
        f.write("#include <string>\n#include <map>\n\n")
        f.write(content)
    print(f"[+] Success: {filename}")