import os
import importlib
import adif_config
import re

def run_all_parsers():
    """
    run all parse_*.py
    """
    parser_files = [f[:-3] for f in os.listdir('.') if f.startswith('parse_') and f.endswith('.py')]
    
    generated_headers = []
    
    print(f"[!] running {len(parser_files)} parse_*.py...")

    for module_name in parser_files:
        try:
            module = importlib.import_module(module_name)
            
            enum_name = module_name.replace('parse_', '')
            func_name = f"generate_{enum_name}_map"
            
            if hasattr(module, func_name):
                print(f"[*] running: {module_name}.{func_name}()")
                getattr(module, func_name)()
                generated_headers.append(f"{enum_name}_map.h")
            else:
                funcs = [f for f in dir(module) if f.startswith('generate_') and f.endswith('_map')]
                if funcs:
                    getattr(module, funcs[0])()
                    generated_headers.append(f"{enum_name}_map.h")
                else:
                    print(f"[-] skip {module_name}: no generate function")
                    
        except Exception as e:
            print(f"[!] Failed to run {module_name} : {e}")

    generate_master_header(generated_headers)

def generate_master_header(headers):
    all_external_includes = set()
    cleaned_headers_info = []

    include_pattern = re.compile(r'^\s*#include\s*[<"].+?[>"]', re.MULTILINE)

    for h in sorted(headers):
        if os.path.exists(h):
            with open(h, 'r', encoding='utf-8') as f:
                content = f.read()
                includes = include_pattern.findall(content)
                for inc in includes:
                    all_external_includes.add(inc.strip())
                
                cleaned_headers_info.append(h)

    # --- Master Header ---
    content = "/**\n * ADIF Constants - Automatically Generated\n"
    content += f" * Source of Truth: {adif_config.ADIF_URL}\n */\n\n"
    content += "#ifndef ADIF_CONSTANTS_H\n"
    content += "#define ADIF_CONSTANTS_H\n\n"

    if all_external_includes:
        content += "// External Dependencies\n"
        for inc in sorted(list(all_external_includes)):
            content += f"{inc}\n"
        content += "\n"
    
    for h in cleaned_headers_info:
        content += f'#include "{h}"\n'
    
    content += "#endif // ADIF_CONSTANTS_H\n"

    with open("adif_constants.h", "w", encoding="utf-8") as f:
        f.write(content)
    print(f"\n[+] Complete: adif_constants.h ({len(headers)} headers integrated)")

if __name__ == "__main__":
    run_all_parsers()