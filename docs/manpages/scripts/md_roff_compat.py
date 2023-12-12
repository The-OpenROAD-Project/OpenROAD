# Usage: 
# cd <OPENROAD_ROOT>/docs/manpages
# python md_roff_compat.py
import re

# prototype with ppl docs.

# original docs
#docs = ["../../src/ppl/README.md"]

# edited docs
docs = ["./src/ppl.md"]

# identify key section and stored in ManPage class. TODO.
class ManPage():
    def __init__(self):
        self.function_name = ""

        pass

def extract_headers(text, level = 1):
    assert isinstance(level, int) and level >= 1
    pattern = r'^#{%d}\s+(.*)$' % level
    headers = re.findall(pattern, text, flags=re.MULTILINE)
    # TODO: Handle developer commands
    headers.remove("Useful Developer Commands")
    return headers

def extract_description(text):
    # this is so that it always tries to match the longer headers first, to disambiguate
    sorted_headers = sorted(extract_headers(text,3), key=len, reverse=True)
    headers = "|".join(re.escape(x) for x in sorted_headers)
    pattern = rf'### ({headers})(.*?)```tcl'
    custom_strings = re.findall(pattern, text, flags=re.DOTALL)
    return [custom_string[1] for custom_string in custom_strings]

def extract_tcl_code(text):
    pattern = r'```tcl\s+(.*?)```'
    tcl_code_matches = re.findall(pattern, text, flags=re.DOTALL)
    # remove the last tcl match
    tcl_code_matches = [x for x in tcl_code_matches if "./test/gcd.tcl" not in x]
    return tcl_code_matches

def extract_tables(text):
    # Find all lines that start with "|"
    table_pattern = r'^\s*\|.*$'
    table_matches = re.findall(table_pattern, text, flags=re.MULTILINE)

    return table_matches



# sections to populate
# 1) Header (starts and ends with ---) title, author, date
# 2) NAME
# 3) SYNOPSIS
# 4) DESCRIPTION
# 5) OPTIONS
# 6) ARGUMENTS
# 7) EXAMPLES
# 8) ENVIRONMENT
# 9) FILES
# 10) SEE ALSO
# 11) HISTORY
# 12) BUGS
# 13) COPYRIGHT

for doc in docs:
    text = open(doc).read()

    # function names
    func_names = extract_headers(text, 3)
    func_names = ["_".join(s.lower().split()) for s in func_names]

    # function description
    func_descs = extract_description(text)

    # synopsis content
    func_synopsis = extract_tcl_code(text)

    # switch names
    switch_names = extract_tables(text)
    idx = 0
    func_switches, tmp = [], []
    for s in switch_names:
        # TODO: Handle developer commands
        if "Command Name" in s:
            break
        if "Switch Name" in s:
            if tmp: func_switches.append(tmp)
            tmp = []
        tmp.append(s)
    if tmp: func_switches.append(tmp)

# grouping the parsed outputs together
results = []
offset_switch_idx = 0

for func_id in range(len(func_synopsis)):
    temp = {}

    temp["name"] = func_names[func_id]
    temp["desc"] = func_descs[func_id]

    # logic if synopsis is one liner, it means that it has no switches
    temp["synopsis"] = func_synopsis[func_id]
    if len(temp["synopsis"]) == 3: offset_switch_idx += 1

    temp["switches"] = func_switches[offset_switch_idx + func_id]

    results.append(temp)
print(len(results))