import os
import glob
import re
from extract_utils import extract_tcl_code, extract_help, extract_proc

# Test objective: Make sure similar output in all three: help, proc, and readme
path = os.getcwd()
module_dir = os.path.dirname(path)
module = os.path.basename(module_dir)

or_home = os.path.dirname(os.path.dirname(os.path.dirname(path)))
os.chdir(or_home)

help_dict, proc_dict, readme_dict = {}, {}, {}

# Directories to exclude (according to md_roff_compat)
exclude = ["sta"]
include = ["./src/odb/src/db/odb.tcl"]

for path in glob.glob("./src/*/src/*tcl") + include:
    # exclude all dirs other than the current dir.
    if module not in path:
        continue

    # exclude these dirs which are not compiled in man (do not have readmes).
    tool_dir = os.path.dirname(os.path.dirname(path))
    if module not in tool_dir:
        continue
    if "odb" in tool_dir:
        tool_dir = "./src/odb"
    if not os.path.exists(os.path.join(tool_dir, "README.md")):
        continue
    if re.search(f".*{'|'.join(e for e in exclude)}.*", path):
        continue

    with open(path, encoding="utf-8") as f:
        # Help patterns
        content = f.read()
        matches = extract_help(content)
        help_dict[tool_dir] = len(matches)

        # Proc patterns
        matches = extract_proc(content)
        proc_dict[tool_dir] = len(matches)

for path in glob.glob("./src/*/README.md"):
    # exclude all dirs other than the current dir.
    if module not in path:
        continue

    if re.search(f".*{'|'.join(e for e in exclude)}.*", path):
        continue
    tool_dir = os.path.dirname(path)

    # for gui, filter out the gui:: for separate processing
    with open(path, encoding="utf-8") as f:
        results = [x for x in extract_tcl_code(f.read()) if "gui::" not in x]
    readme_dict[tool_dir] = len(results)

    # for pad, remove `make_fake_io_site` because it is a hidden cmd arg
    if "pad" in tool_dir:
        readme_dict[tool_dir] -= 1

print(
    "Tool Dir".ljust(20), "Help count".ljust(15), "Proc count".ljust(15), "Readme count"
)

for tool_dir in help_dict:
    h, p, r = (
        help_dict[tool_dir],
        proc_dict.get(tool_dir, 0),
        readme_dict.get(tool_dir, 0),
    )
    print(tool_dir.ljust(20), str(h).ljust(15), str(p).ljust(15), str(r))
    if h == p == r:
        print("Command counts match.")
    else:
        print("Command counts do not match.")
