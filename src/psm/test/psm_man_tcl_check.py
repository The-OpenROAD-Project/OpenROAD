import os
import unittest

from man_tcl_check import count_commands


class ManTclCheck(unittest.TestCase):
    def test_command_counts_match(self):
        test_dir = os.path.dirname(os.path.abspath(__file__))
        module_dir = os.path.dirname(test_dir)
        h, p, r = count_commands(module_dir)
        self.assertEqual(h, 7, f"help count ({h}) != expected (7)")
        self.assertEqual(p, 7, f"proc count ({p}) != expected (7)")
        self.assertEqual(r, 8, f"readme count ({r}) != expected (8)")


if __name__ == "__main__":
    unittest.main()
