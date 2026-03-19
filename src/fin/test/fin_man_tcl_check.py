import os
import unittest

from man_tcl_check import count_commands

HELP_COUNT = 1
PROC_COUNT = 1
README_COUNT = 1


class ManTclCheck(unittest.TestCase):
    def test_command_counts_match(self):
        test_dir = os.path.dirname(os.path.abspath(__file__))
        module_dir = os.path.dirname(test_dir)
        h, p, r = count_commands(module_dir)
        self.assertEqual(h, HELP_COUNT, f"help count ({h}) != expected ({HELP_COUNT})")
        self.assertEqual(p, PROC_COUNT, f"proc count ({p}) != expected ({PROC_COUNT})")
        self.assertEqual(
            r, README_COUNT, f"readme count ({r}) != expected ({README_COUNT})"
        )


if __name__ == "__main__":
    unittest.main()
