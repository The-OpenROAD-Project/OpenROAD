"""
Usage: 
cd <OPENROAD_ROOT>/docs/manpages
python md_roff_compat.py
"""

import re
import io
import datetime

# list of edited docs
SRC_DIR = "./md/man2"
# problematic: drt, gpl, grt, gui, ifp, mpl2, pdn, psm
# psm: need to handle the table wrt the #### Options, not all tables. 
# also you need to change the ### FUNCTION_NAME parsing. Sometimes the 
#    function name could be something weird like `diff_spef` or `pdngen`
#    so it would be better to have a more informative header for the RTD docs. 
# rmp: many level 3 headers
# sta: documentation is hosted elsewhere. (not currently in RTD also.)
# 
tools = ["ant", "cts", "dft", "dpl", "fin", "pad", "par", "ppl", "rsz",\
            "tap", "upf"]
#tools = ["rsz"]
docs = [f"{SRC_DIR}/{tool}.txt" for tool in tools]

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
        # it is okay for a function to have no switches.
        #assert self.switches, print("func switches not set")
        filepath = f"{SRC_DIR}/{self.name}.md"
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
        if not self.switches:
            f.write(f"\nThis command has no switches.\n")
        for key, val in self.switches.items():
            f.write(f"\n`{key}`: {val}\n")

    def write_placeholder(self, f):
        assert isinstance(f, io.TextIOBase) and\
         f.writable(), "File pointer is not open for writing."

        # TODO: these are all not populated currently, not parseable from docs. 
        # TODO: Arguments can actually be parsed, but you need to preprocess the synopsis further. 
        sections = ["ARGUMENTS", "EXAMPLES", "SEE ALSO"]
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
    if "Useful Developer Commands" in headers: headers.remove("Useful Developer Commands")
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

    # Exclude matches containing HTML tags
    table_matches = [table for table in table_matches if not re.search(r'<.*?>', table)]

    return table_matches

if __name__ == "__main__":
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

        print(len(func_names)); print(func_names)
        print(len(func_descs)); print(func_descs)
        print(len(func_synopsis)); print(func_synopsis)
        print(len(func_switches)); print(func_switches)

        # grouping the parsed outputs together
        offset_switch_idx = 0
        for func_id in range(len(func_synopsis)):
            temp = ManPage()

            temp.name = func_names[func_id]
            temp.desc = func_descs[func_id]
            temp.synopsis = func_synopsis[func_id]

            # logic if synopsis is one liner, it means that it has no switches
            if len(temp.synopsis.split("\n")) == 2:
                temp.write_roff_file()
                offset_switch_idx += 1
                continue


            switches = func_switches[func_id - offset_switch_idx]
            switches_dict = {}
            for idx, x in enumerate(switches):
                # Skip header and | --- | dividers.
                if idx == 0: continue
                if "---" in x: continue
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