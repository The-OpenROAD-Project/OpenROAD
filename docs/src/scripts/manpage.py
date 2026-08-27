## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2024-2026, The OpenROAD Authors

# This code describes the ManPage class, which consists of the data classes
#  as well as the code needed to write the roff-compatible manpage file.

from __future__ import annotations

import io
import datetime
import os
import tempfile


def page_date(source=None):
    """
    Date stamped into a page's footer, as yy/mm/dd.

    Stamping the build date made every generated page differ from one day to
    the next, so the man page build never hit its Bazel cache across a date
    boundary even with unchanged sources. Prefer SOURCE_DATE_EPOCH (the
    reproducible-builds convention -- set it to pin output exactly), then the
    modification time of the document the page is generated from, which is
    what the footer date is meant to convey. `now` remains the fallback for
    callers that construct a page from no file at all.
    """
    epoch = os.environ.get("SOURCE_DATE_EPOCH", "").strip()
    if epoch:
        # Reject a malformed pin rather than falling through to a date that
        # varies per machine: the caller asked for reproducible output, and
        # silently not delivering it is worse than stopping.
        if not epoch.isdigit():
            raise ValueError(
                f"SOURCE_DATE_EPOCH must be a Unix timestamp, got {epoch!r}"
            )
        stamp = datetime.datetime.fromtimestamp(int(epoch), datetime.timezone.utc)
    elif source and os.path.exists(source):
        stamp = datetime.datetime.fromtimestamp(
            os.path.getmtime(source), datetime.timezone.utc
        )
    else:
        # UTC like the two branches above, so the stamp does not depend on the
        # builder's timezone.
        stamp = datetime.datetime.now(datetime.timezone.utc)
    return stamp.strftime("%y/%m/%d")


# identify key section and stored in ManPage class.
class ManPage:
    def __init__(self, man_level=2, source=None):
        assert man_level in [2, 3], "only writable for man2/man3"
        self.name = ""
        self.desc = ""
        self.synopsis = ""
        self.switches = {}
        self.args = {}
        self.examples = []  # List of example dictionaries
        self.see_also = []  # List of related commands/functions
        self.datetime = page_date(source)
        self.man_level = f"man{man_level}"

    def add_example(self, description: str, code: str, output: str | None = None):
        """
        Add an example to the manpage.

        Args:
            description: Description of what the example does
            code: The actual code/command example
            output: Optional expected output
        """
        example = {"description": description, "code": code, "output": output}
        self.examples.append(example)

    def add_see_also(
        self, reference: str, section: int = None, description: str | None = None
    ):
        """
        Add a reference to the SEE ALSO section.

        Args:
            reference: Name of the command/function to reference
            section: Manual section number (optional)
            description: Brief description of the reference (optional)
        """
        see_also_entry = {
            "reference": reference,
            "section": section,
            "description": description,
        }
        self.see_also.append(see_also_entry)

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
            self.write_examples(f)
            self.write_see_also(f)
            self.write_copyright(f)

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
        else:
            for key, val in self.switches.items():
                f.write(f"\n`{key}`: {val}\n")

    def write_arguments(self, f):
        assert (
            isinstance(f, io.TextIOBase) and f.writable()
        ), "File pointer is not open for writing."

        f.write(f"\n# ARGUMENTS\n")
        if not self.args:
            f.write(f"\nThis command has no arguments.\n")
        else:
            for key, val in self.args.items():
                f.write(f"\n`{key}`: {val}\n")

    def write_examples(self, f):
        """Write the EXAMPLES section with proper formatting."""
        assert (
            isinstance(f, io.TextIOBase) and f.writable()
        ), "File pointer is not open for writing."

        f.write(f"\n# EXAMPLES\n")

        if not self.examples:
            f.write(f"No examples available.\n")
        else:
            for example in self.examples:
                f.write(f"{example['description']}\n")
                if example.get("code"):
                    f.write(f"{example['code']}\n")
                if example.get("output"):
                    f.write(f"\n{example['output']}\n")

    def write_see_also(self, f):
        """Write the SEE ALSO section with proper formatting."""
        assert (
            isinstance(f, io.TextIOBase) and f.writable()
        ), "File pointer is not open for writing."

        f.write(f"\n# SEE ALSO\n")

        if not self.see_also:
            f.write(f"No related references.\n")
        else:
            for ref in self.see_also:
                reference = ref["reference"]
                f.write(f"{reference}\n")

    def write_copyright(self, f):
        assert (
            isinstance(f, io.TextIOBase) and f.writable()
        ), "File pointer is not open for writing."

        f.write(f"\n# COPYRIGHT\n\n")
        f.write(f"Copyright (c) 2024, The OpenROAD Authors.\n")


# Example usage demonstration
if __name__ == "__main__":
    # Create a sample manpage
    man = ManPage(man_level=2)
    man.name = "sample_function"
    man.desc = (
        "A sample function that demonstrates the enhanced ManPage class capabilities."
    )
    man.synopsis = "int sample_function(int arg1, char *arg2, int flags);"

    # Add switches and arguments
    man.switches = {
        "-v, --verbose": "Enable verbose output",
        "-h, --help": "Display help message",
    }

    man.args = {
        "arg1": "Integer argument for processing",
        "arg2": "String argument containing input data",
        "flags": "Bitwise flags for operation control",
    }

    # Add examples
    man.add_example(
        "Basic usage with minimal arguments",
        'result = sample_function(42, "hello", 0);',
        "Returns: 0 on success",
    )

    man.add_example(
        "Usage with verbose flag enabled",
        'result = sample_function(100, "test data", VERBOSE_FLAG);',
        "Returns: 0 on success, with detailed logging",
    )

    # Add see also references
    man.add_see_also(
        "related_function", 2, "Similar function with different parameters"
    )
    man.add_see_also("utility_helper", 3, "Helper function for data processing")
    man.add_see_also("debugging_tools", description="Collection of debugging utilities")

    # Create a temporary directory for the output
    with tempfile.TemporaryDirectory() as temp_dir:
        man.write_roff_file(temp_dir)

        # Read and print the generated content
        filepath = os.path.join(temp_dir, f"{man.name}.md")
        with open(filepath, "r") as f:
            generated_content = f.read()

        print("Generated man page content:")
        print("=" * 50)
        print(generated_content)
        print("=" * 50)
