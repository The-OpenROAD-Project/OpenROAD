import io
import datetime

# identify key section and stored in ManPage class. 
class ManPage():
    def __init__(self):
        self.name = ""
        self.desc = ""
        self.synopsis = ""
        self.switches = {}
        self.args = {}
        self.datetime = datetime.datetime.now().strftime("%y/%m/%d")

    def write_roff_file(self, dst_dir = './md/man2'):
        assert self.name, print("func name not set")
        assert self.desc, print("func desc not set")
        assert self.synopsis, print("func synopsis not set")
        # it is okay for a function to have no switches.
        #assert self.switches, print("func switches not set")
        filepath = f"{dst_dir}/{self.name}.md"
        man_level = dst_dir.split("/")[-1]
        with open(filepath, "w") as f:
            self.write_header(f, man_level)
            self.write_name(f)
            self.write_synopsis(f)
            self.write_description(f)
            self.write_options(f)
            self.write_arguments(f)
            self.write_placeholder(f) #TODO.
            self.write_copyright(f)
    
    def write_header(self, f, man_level):
        print(man_level)
        assert isinstance(f, io.TextIOBase) and\
         f.writable(), "File pointer is not open for writing."
        assert man_level == "man1" or man_level == "man2" or\
                 man_level == "man3", print("must be either man1|man2|man3")

        f.write(f"---\n")
        f.write(f"title: {self.name}({man_level[-1]})\n")
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

    def write_arguments(self, f):
        assert isinstance(f, io.TextIOBase) and\
         f.writable(), "File pointer is not open for writing."

        f.write(f"\n# ARGUMENTS\n")
        if not self.args:
            f.write(f"\nThis command has no arguments.\n")
        for key, val in self.args.items():
            f.write(f"\n`{key}`: {val}\n")


    def write_placeholder(self, f):
        assert isinstance(f, io.TextIOBase) and\
         f.writable(), "File pointer is not open for writing."

        # TODO: these are all not populated currently, not parseable from docs. 
        # TODO: Arguments can actually be parsed, but you need to preprocess the synopsis further. 
        sections = ["EXAMPLES", "SEE ALSO"]
        for s in sections:
            f.write(f"\n# {s}\n")

    def write_copyright(self, f):
        assert isinstance(f, io.TextIOBase) and\
         f.writable(), "File pointer is not open for writing."

        f.write(f"\n# COPYRIGHT\n\n")
        f.write(f"Copyright (c) 2024, The Regents of the University of California. All rights reserved.\n")

if __name__ == "__main__":
    pass