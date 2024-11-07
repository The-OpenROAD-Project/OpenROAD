import os
import glob
import re
from extract_utils import extract_tcl_code, extract_help, extract_proc
from extract_utils import (
    clean_whitespaces,
    clean_parse_syntax,
    check_function_signatures,
)

# Test objective: Make sure similar output in all three: help, proc, and readme

# Store results
help_dict, proc_dict, readme_dict = {}, {}, {}

# Directories to exclude (according to md_roff_compat)
exclude = ["sta"]
include = ["./src/odb/src/db/odb.tcl"]

for path in glob.glob("./src/*/src/*tcl") + include:
    # exclude these dirs which are not compiled in man (do not have readmes).
    tool_dir = os.path.dirname(os.path.dirname(path))
    if "odb" in tool_dir:
        tool_dir = "./src/odb"
    if not os.path.exists(f"{tool_dir}/README.md"):
        continue
    if re.search(f".*{'|'.join(e for e in exclude)}.*", path):
        continue

    # special handling for pad, since it has 3 Tcls.
    if "ICeWall" in path or "PdnGen" in path:
        continue

    with open(path) as f:
        # Help patterns
        content = f.read()
        matches = extract_help(content)

        for match in matches:
            cmd, rest = match[0], match[1]
            cmd, rest = clean_whitespaces(cmd), clean_whitespaces(rest)
            help_dict[cmd] = rest

        # Proc patterns
        matches = extract_proc(content)

        for match in matches:
            cmd, keys, flags = match[0], match[1], match[2]
            cmd, rest = clean_whitespaces(cmd), clean_whitespaces(keys + " " + flags)
            rest = clean_parse_syntax(rest)
            proc_dict[cmd] = rest

for path in glob.glob("./src/*/README.md"):
    if re.search(f".*{'|'.join(e for e in exclude)}.*", path):
        continue
    tool_dir = os.path.dirname(path)

    # for gui, filter out the gui:: for separate processing
    matches = [x for x in extract_tcl_code(open(path).read()) if "gui::" not in x]

    for match in matches:
        cmd = match.split()[0]
        rest = " ".join(match.split()[1:])
        readme_dict[cmd] = rest

succeeded = 0
for cmd in help_dict:
    print("----------")
    print(cmd)
    isValid = True
    if cmd not in help_dict:
        print("command not parsed in help_dict")
        isValid = False
    if cmd not in proc_dict:
        print("command not parsed in proc_dict")
        isValid = False
    if cmd not in readme_dict:
        print("command not parsed in readme_dict")
        isValid = False

    # Test switches here
    try:
        s1, s2, s3 = help_dict[cmd], proc_dict[cmd], readme_dict[cmd]
        res1, res2, res3 = (
            check_function_signatures(s1, s2),
            check_function_signatures(s1, s3),
            check_function_signatures(s2, s3),
        )
        assert res1 and res2 and res3, print(
            f"Help/Proc: {res1}\nHelp/Rdme: {res2}\nProc/Rdme: {res3}"
        )
        succeeded += 1
        print("Success.")
    except Exception as e:
        print("Failed.")
        print(f"Help Dict: {help_dict.get(cmd, 'Not found')}")
        print(f"Proc Dict: {proc_dict.get(cmd, 'Not found')}")
        print(f"Readme Dict: {readme_dict.get(cmd, 'Not found')}")

print(f"----------\nSucceeded: {succeeded} out of {len(help_dict)} tests.")
assert succeeded == len(help_dict), "Keys/flags are missing in one of Help/Proc/Readme."
