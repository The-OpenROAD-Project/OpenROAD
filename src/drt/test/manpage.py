###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2024, The Regents of the University of California
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
###############################################################################

# This code describes the ManPage class, which consists of the data classes
#  as well as the code needed to write the roff-compatible manpage file.

import io
import datetime


# identify key section and stored in ManPage class.
class ManPage:
    def __init__(self, man_level=2):
        assert man_level in [2, 3], "only writable for man2/man3"
        self.name = ""
        self.desc = ""
        self.synopsis = ""
        self.switches = {}
        self.args = {}
        self.datetime = datetime.datetime.now().strftime("%y/%m/%d")
        self.man_level = f"man{man_level}"

    def write_roff_file(self, dst_dir="./md/man2"):
        assert self.name, print("func name not set")
        assert self.desc, print("func desc not set")
        assert self.synopsis, print("func synopsis not set")
        # it is okay for a function to have no switches.
        # assert self.switches, print("func switches not set")
        filepath = f"{dst_dir}/{self.name}.md"
        with open(filepath, "w") as f:
            self.write_header(f)
            self.write_name(f)
            self.write_synopsis(f)
            self.write_description(f)
            self.write_options(f)
            self.write_arguments(f)
            self.write_placeholder(f)  # TODO.

    def write_header(self, f):
        assert (
            isinstance(f, io.TextIOBase) and f.writable()
        ), "File pointer is not open for writing."

        f.write(f"---\n")
        f.write(f"title: {self.name}({self.man_level[-1]})\n")
        f.write(f"date: {self.datetime}\n")
        f.write(f"---\n")

    def write_name(self, f):
        assert (
            isinstance(f, io.TextIOBase) and f.writable()
        ), "File pointer is not open for writing."

        f.write(f"\n# NAME\n\n")
        f.write(f"{self.name} - {' '.join(self.name.split('_'))}\n")

    def write_synopsis(self, f):
        assert (
            isinstance(f, io.TextIOBase) and f.writable()
        ), "File pointer is not open for writing."

        f.write(f"\n# SYNOPSIS\n\n")
        f.write(f"{self.synopsis}\n")

    def write_description(self, f):
        assert (
            isinstance(f, io.TextIOBase) and f.writable()
        ), "File pointer is not open for writing."

        f.write(f"\n# DESCRIPTION\n\n")
        f.write(f"{self.desc}\n")

    def write_options(self, f):
        assert (
            isinstance(f, io.TextIOBase) and f.writable()
        ), "File pointer is not open for writing."

        f.write(f"\n# OPTIONS\n")
        if not self.switches:
            f.write(f"\nThis command has no switches.\n")
        for key, val in self.switches.items():
            f.write(f"\n`{key}`: {val}\n")

    def write_arguments(self, f):
        assert (
            isinstance(f, io.TextIOBase) and f.writable()
        ), "File pointer is not open for writing."

        f.write(f"\n# ARGUMENTS\n")
        if not self.args:
            f.write(f"\nThis command has no arguments.\n")
        for key, val in self.args.items():
            f.write(f"\n`{key}`: {val}\n")

    def write_placeholder(self, f):
        assert (
            isinstance(f, io.TextIOBase) and f.writable()
        ), "File pointer is not open for writing."

        # TODO: these are all not populated currently, not parseable from docs.
        # TODO: Arguments can actually be parsed, but you need to preprocess the synopsis further.
        sections = ["EXAMPLES", "SEE ALSO"]
        for s in sections:
            f.write(f"\n# {s}\n")

    def write_copyright(self, f):
        assert (
            isinstance(f, io.TextIOBase) and f.writable()
        ), "File pointer is not open for writing."

        f.write(f"\n# COPYRIGHT\n\n")
        f.write(
            f"Copyright (c) 2024, The Regents of the University of California. All rights reserved.\n"
        )


if __name__ == "__main__":
    pass
