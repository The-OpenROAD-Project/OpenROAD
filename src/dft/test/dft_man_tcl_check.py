import os
import unittest

from man_tcl_check import count_commands


class ManTclCheck(unittest.TestCase):
    def test_command_counts_match(self):
        test_dir = os.path.dirname(os.path.abspath(__file__))
        module_dir = os.path.dirname(test_dir)
        h, p, r = count_commands(module_dir)
        self.assertEqual(h, p, f"help count ({h}) != proc count ({p})")
        self.assertEqual(p, r, f"proc count ({p}) != readme count ({r})")


if __name__ == "__main__":
    unittest.main()
