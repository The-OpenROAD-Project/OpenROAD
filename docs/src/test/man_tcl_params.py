import os
import glob
import re
from extract_utils import extract_tcl_code, extract_help, extract_proc

def clean_whitespaces(text):
    tmp = text.strip().replace("\\", "").replace("\n", "")
    return " ".join(tmp.split())

def clean_parse_syntax(text):
    tmp = text.replace("keys", "").replace("flags", "")\
            .replace("{", "").replace("}", "")
    return ' '.join([f'[{option}]' for option in tmp.split()])

def check_function_signatures(text1, text2):
    set1 = set(match.group(1) for \
                match in re.finditer(r'\[([^\]]+)\]', text1))
    set1 = {x.split()[0] for x in set1}
    
    set2 = set(match.group(1) for \
                match in re.finditer(r'\[([^\]]+)\]', text2))
    set2 = {x.split()[0] for x in set2}

    if set1 == set2: return True
    print(set1)
    print(set2)
    return False

# Test this tool
tool = "cts"

# Test objective: Make sure similar output in all three: help, proc, and readme
path = os.path.realpath("md_roff_compat.py")
or_home = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(path))))
os.chdir(or_home)

# Regexes
help_dict, proc_dict, readme_dict = {}, {}, {}  

# Directories to exclude (according to md_roff_compat)
exclude = ["sta"]
include = ["./src/odb/src/db/odb.tcl"]

for path in glob.glob("./src/*/src/*tcl") + include:
    # exclude these dirs which are not compiled in man (do not have readmes).
    tool_dir = os.path.dirname(os.path.dirname(path))
    if "odb" in tool_dir: tool_dir = './src/odb'
    if not os.path.exists(f"{tool_dir}/README.md"): continue
    if re.search(f".*{'|'.join(e for e in exclude)}.*", path): continue

    # special handling for pad, since it has 3 Tcls.
    if "ICeWall" in path or "PdnGen" in path: continue

    with open(path) as f:
        if tool not in path: continue

        # Help patterns
        content = f.read()
        matches = extract_help(content)
        
        for match in matches:
            cmd, rest = match[0], match[1]
            cmd, rest = clean_whitespaces(cmd), clean_whitespaces(rest)
            help_dict[cmd] = rest
            #print(cmd, rest)

        # Proc patterns
        matches = extract_proc(content)

        for match in matches:
            cmd, keys, flags = match[0], match[1], match[2]
            cmd, rest = clean_whitespaces(cmd), clean_whitespaces(keys + " " + flags)
            rest = clean_parse_syntax(rest)
            proc_dict[cmd] = rest
            # print(cmd, rest)


for path in glob.glob("./src/*/README.md"):
    if re.search(f".*{'|'.join(e for e in exclude)}.*", path): continue
    tool_dir = os.path.dirname(path)

    if tool not in path: continue

    # for gui, filter out the gui:: for separate processing
    matches = [x for x in extract_tcl_code(open(path).read()) if "gui::" not in x]

    # for pad, remove `make_fake_io_site`
    if 'pad' in tool_dir:
        matches = [x for x in matches if "make_fake_io_site" not in x]

    for match in matches:
        cmd = match.split()[0]
        rest = " ".join(match.split()[1:])
        readme_dict[cmd] = rest
        # print(cmd, rest)

for cmd in help_dict:
    print(cmd)
    isValid = True
    if cmd not in help_dict: print("command not parsed in help_dict"); isValid = False
    if cmd not in proc_dict: print("command not parsed in proc_dict"); isValid = False
    if cmd not in readme_dict: print("command not parsed in readme_dict"); isValid = False

    # If invalid, don't need to test further.
    if not isValid: 
        print("Skipping keys/flag testing")
        continue

    # Test switches here
    s1, s2, s3 = help_dict[cmd], proc_dict[cmd], readme_dict[cmd]
    print(f"Help/Proc: {check_function_signatures(s1,s2)}")
    print(f"Help/Rdme: {check_function_signatures(s1,s3)}")
    print(f"Proc/Rdme: {check_function_signatures(s2,s3)}")