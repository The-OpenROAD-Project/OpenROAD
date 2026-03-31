import glob
import os
import re
import unittest

from extract_utils import extract_help, extract_proc, extract_tcl_code


class TestManTclCheck(unittest.TestCase):
    """Test that command counts match across help, proc, and readme."""

    def test_command_counts_match(self):
        module = os.path.basename(__file__).replace("_man_tcl_check.py", "")
        script_dir = os.path.dirname(os.path.abspath(__file__))
        or_home = os.path.dirname(os.path.dirname(os.path.dirname(script_dir)))
        os.chdir(or_home)

        help_dict, proc_dict, readme_dict = {}, {}, {}

        exclude = ["sta"]
        include = ["./src/odb/src/swig/tcl/odb.tcl"]

        for path in sorted(glob.glob("./src/*/src/*tcl")) + include:
            if module not in path:
                continue
            tool_dir = os.path.dirname(os.path.dirname(path))
            if module not in tool_dir:
                continue
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
                content = f.read()
                help_dict[tool_dir] = help_dict.get(tool_dir, 0) + len(
                    extract_help(content)
                )
                proc_dict[tool_dir] = proc_dict.get(tool_dir, 0) + len(
                    extract_proc(content)
                )

        for path in glob.glob("./src/*/README.md"):
            if module not in path:
                continue
            if re.search(f".*{'|'.join(e for e in exclude)}.*", path):
                continue
            tool_dir = os.path.dirname(path)
            with open(path) as f:
                results = [
                    x
                    for x in extract_tcl_code(f.read())
                    if "gui::" not in x and "utl::" not in x
                ]
            readme_dict[tool_dir] = len(results)

        for tool_dir in help_dict:
            h = help_dict[tool_dir]
            p = proc_dict[tool_dir]
            r = readme_dict[tool_dir]
            self.assertEqual(
                h,
                p,
                f"{tool_dir}: help count ({h}) != proc count ({p})",
            )
            self.assertEqual(
                h,
                r,
                f"{tool_dir}: help count ({h}) != readme count ({r})",
            )


if __name__ == "__main__":
    unittest.main()
