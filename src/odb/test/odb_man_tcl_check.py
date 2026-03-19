import os
import unittest

from man_tcl_check import count_commands


class ManTclCheck(unittest.TestCase):
    def test_command_counts_match(self):
        test_dir = os.path.dirname(os.path.abspath(__file__))
        module_dir = os.path.dirname(test_dir)
        h, p, r = count_commands(module_dir)
        self.assertEqual(h, 21, f"help count ({h}) != expected (21)")
        self.assertEqual(p, 23, f"proc count ({p}) != expected (23)")
        self.assertEqual(r, 22, f"readme count ({r}) != expected (22)")


if __name__ == "__main__":
    unittest.main()
