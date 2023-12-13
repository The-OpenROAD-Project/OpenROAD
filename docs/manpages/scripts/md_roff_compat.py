"""
Usage: 
cd <OPENROAD_ROOT>/docs/manpages
python md_roff_compat.py
"""

import re
import io
import datetime

# original docs
#docs = ["../../src/ppl/README.md"]

# edited docs
docs = ["./src/man2/ppl.txt"]

# identify key section and stored in ManPage class. 
class ManPage():
    def __init__(self):
        self.name = ""
        self.desc = ""
        self.synopsis = ""
        self.switches = {}
        self.datetime = datetime.datetime.now().strftime("%y/%m/%d")

    def write_roff_file(self):
        assert self.name, print("func name not set")
        assert self.desc, print("func desc not set")
        assert self.synopsis, print("func synopsis not set")
        assert self.switches, print("func switches not set")
        filepath = f"./src/man2/{self.name}.md"
        with open(filepath, "w") as f:
            self.write_header(f)
            self.write_name(f)
            self.write_synopsis(f)
            self.write_description(f)
            self.write_options(f)
            self.write_placeholder(f) #TODO.
            self.write_copyright(f)
    
    def write_header(self, f):
        assert isinstance(f, io.TextIOBase) and\
         f.writable(), "File pointer is not open for writing."

        f.write(f"---\n")
        f.write(f"title: {self.name}(2)\n")
        f.write(f"author: Jack Luar (TODO@TODO.com)\n")
        f.write(f"date: {self.datetime}\n")
        f.write(f"---\n")

    def write_name(self, f):
        assert isinstance(f, io.TextIOBase) and\
         f.writable(), "File pointer is not open for writing."

        f.write(f"\n# NAME\n\n")
        f.write(f"{self.name} - {' '.join(self.name.split('_'))}\n")

    def write_synopsis(self, f):
        assert isinstance(f, io.TextIOBase) and\
         f.writable(), "File pointer is not open for writing."

        f.write(f"\n# SYNOPSIS\n\n")
        f.write(f"{self.synopsis}\n")


    def write_description(self, f):
        assert isinstance(f, io.TextIOBase) and\
         f.writable(), "File pointer is not open for writing."

        f.write(f"\n# DESCRIPTION\n\n")
        f.write(f"{self.desc}\n")

    def write_options(self, f):
        assert isinstance(f, io.TextIOBase) and\
         f.writable(), "File pointer is not open for writing."

        f.write(f"\n# OPTIONS\n")
        for key, val in self.switches.items():
            f.write(f"\n`{key}`: {val}\n")

    def write_placeholder(self, f):
        assert isinstance(f, io.TextIOBase) and\
         f.writable(), "File pointer is not open for writing."

        # TODO: these are all not populated currently, not parseable from docs. 
        # TODO: Arguments can actually be parsed, but you need to preprocess the synopsis further. 
        sections = ["ARGUMENTS", "EXAMPLES", "ENVIRONMENT", "FILES", "SEE ALSO", "HISTORY", "BUGS"]
        for s in sections:
            f.write(f"\n# {s}\n")

    def write_copyright(self, f):
        assert isinstance(f, io.TextIOBase) and\
         f.writable(), "File pointer is not open for writing."

        f.write(f"\n# COPYRIGHT\n\n")
        f.write(f"Copyright (c) 2024, The Regents of the University of California. All rights reserved.\n")

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
    return [custom_string[1].strip() for custom_string in custom_strings]

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

for doc in docs:
    text = open(doc).read()

    # function names
    func_names = extract_headers(text, 3)
    func_names = ["_".join(s.lower().split()) for s in func_names]

    # function description
    func_descs = extract_description(text)

    # synopsis content
    func_synopsis = extract_tcl_code(text)

    # switch names (TODO: needs refactoring...)
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
        tmp.append(s.strip())
    if tmp: func_switches.append(tmp)

# grouping the parsed outputs together
offset_switch_idx = 0

for func_id in range(len(func_synopsis)):
    temp = ManPage()

    temp.name = func_names[func_id]
    temp.desc = func_descs[func_id]

    # logic if synopsis is one liner, it means that it has no switches
    temp.synopsis = func_synopsis[func_id]
    if len(temp.synopsis) == 3: offset_switch_idx += 1

    switches = func_switches[offset_switch_idx + func_id]
    switches_dict = {}
    for idx, x in enumerate(switches):
        if idx == 0 or idx == 1: continue
        switch_name = x.split("|")[1]
        # Find the index of the 2nd and last occurrence of "|". Since some content might contain "|"
        second_pipe_index = x.find("|", x.find("|") + 1)
        last_pipe_index = x.rfind("|")
        switch_description = x[second_pipe_index+1: last_pipe_index-1] 
        switch_name = switch_name.replace("`", "").strip()
        switches_dict[switch_name] = switch_description
    temp.switches = switches_dict
    temp.write_roff_file()

print('Ok')