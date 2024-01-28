import os
import glob
import re
from md_roff_compat import exclude2
from extract_utils import extract_tcl_code

path = os.path.realpath("md_roff_compat.py")
or_home = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(path))))
os.chdir(or_home)
# Essentially, anything that exists in help must exist in man AND Readme.md
# Objective for each Tcl: try to parse sta::define_cmd_args {} <= end
# sta::parse_key_args ... keys {}... flags{} <-end 

# Help pattern counts (correct ones are listed)
# ant 1, fin 1, mpl 2, dbsta/readverilog 3, ifp 3, utl 1
# dst 3 (no readme), tap 5, grt 13, gui 7 (problematic..)
# pdn 9, psm 4, stt 1 (no readme), rsz 17
# IOPlacer.tcl missing the sta::define_cmd_args for no args functions.
# rcx 9, pad.tcl what about sta::define_hidden_cmd_args
# upf 8 dpl 1 par 6 mpl2 3 gpl 2 rmp 1
# dft.tcl does not have any sta::define_cmd_args!

# TODO: THose commands inside namespace eval sta need special regex. define_cmd_args. (e.g. dbsta/dbsta)

# Regexes
help_pattern = r'sta::define_cmd_args\s+"(.*?)"\s*{([^}]*)}'
proc_pattern = r'sta::parse_key_args\s+"(.*?)"\s*args\s*(.*?keys.*?})(.*?flags.*?})'
help_dict, proc_dict, readme_dict = {}, {}, {}  

# Directories to exclude (according to md_roff_compat)
exclude = exclude2

total = 0
for path in glob.glob("./src/*/src/*tcl"):
    # exclude these dirs which are not compiled in man (do not have readmes).
    tool_dir = os.path.dirname(os.path.dirname(path))
    if not os.path.exists(f"{tool_dir}/README.md"): continue
    if re.search(f".*{'|'.join(e for e in exclude)}.*", path): continue

    # special handling for pad, since it has 3 Tcls.
    if "ICeWall" in path or "PdnGen" in path: continue

    # Help patterns
    with open(path) as f:
        content = f.read()
        matches = re.findall(help_pattern, content, re.DOTALL)
        #print(path, len(matches))
        help_dict[tool_dir] = len(matches)
        #for match in matches:
        #    help_cmd, help_args = match[0], match[1]

        if not matches: 
            print(tool_dir)
            print('not found.')

    # Proc patterns
    with open(path) as f:
        content = f.read()
        matches = re.findall(proc_pattern, content, re.DOTALL)
        #print(path, len(matches))
        proc_dict[tool_dir] = len(matches)
        #if matches:
        #    for match in matches: print(match)
        #for match in matches:
            #cmd, keys, flags = match.group(1), match.group(2), match.group(3) 

        if not matches: 
            print(tool_dir)
            print('not found.')

for path in glob.glob("./src/*/README.md"):
    if re.search(f".*{'|'.join(e for e in exclude)}.*", path): continue
    tool_dir = os.path.dirname(path)
    readme_dict[tool_dir] = len(extract_tcl_code(open(path).read()))
    total += 1

print(total)

for path in help_dict:
    print(path, help_dict[path], proc_dict[path], readme_dict[path])