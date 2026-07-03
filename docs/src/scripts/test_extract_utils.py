import unittest

from extract_utils import extract_help, extract_proc


class TestExtractHelp(unittest.TestCase):
    """Tests for extract_help() which parses sta::define_cmd_args blocks."""

    def test_basic_command(self):
        text = """
sta::define_cmd_args "my_command" {[-option val]}
proc my_command { args } {
}
"""
        matches = extract_help(text)
        self.assertEqual(len(matches), 1)
        self.assertEqual(matches[0][0], "my_command")

    def test_checker_off_excluded(self):
        text = """
sta::define_cmd_args "hidden_cmd" {[-opt val]} ;# checker off
proc hidden_cmd { args } {
}
"""
        matches = extract_help(text)
        self.assertEqual(len(matches), 0)

    def test_multiline_args(self):
        text = """
sta::define_cmd_args "my_command" {[-option1 val1] \\
                                   [-option2 val2] \\
                                   [-option3]}
proc my_command { args } {
}
"""
        matches = extract_help(text)
        self.assertEqual(len(matches), 1)
        self.assertEqual(matches[0][0], "my_command")

    def test_nested_braces(self):
        text = """
sta::define_cmd_args "save_image" {[-area {x0 y0 x1 y1}] \\
                                   path}
proc save_image { args } {
}
"""
        matches = extract_help(text)
        self.assertEqual(len(matches), 1)
        self.assertEqual(matches[0][0], "save_image")

    def test_proc_redirect_counted(self):
        """Commands using sta::proc_redirect should be counted."""
        text = """
sta::define_cmd_args "report_nets" {[-verbose]}

sta::proc_redirect report_nets {
  sta::parse_key_args "report_nets" args keys {} flags {-verbose}
}
"""
        matches = extract_help(text)
        self.assertEqual(len(matches), 1)
        self.assertEqual(matches[0][0], "report_nets")

    def test_proc_redirect_does_not_consume_next_command(self):
        """A proc_redirect between two commands must not eat the second one."""
        text = """
sta::define_cmd_args "cmd_with_redirect" {[-verbose]} ;# checker off

sta::proc_redirect cmd_with_redirect {
  sta::parse_key_args "cmd_with_redirect" args keys {} flags {-verbose}
}

sta::define_cmd_args "next_command" {}
proc next_command { } {
}
"""
        matches = extract_help(text)
        names = [m[0] for m in matches]
        self.assertIn("next_command", names)
        self.assertNotIn("cmd_with_redirect", names)

    def test_multiple_commands(self):
        text = """
sta::define_cmd_args "cmd_a" {[-opt1]}
proc cmd_a { args } {
}

sta::define_cmd_args "cmd_b" {[-opt2]}
proc cmd_b { args } {
}

sta::define_cmd_args "cmd_c" {[-opt3]} ;# checker off
proc cmd_c { args } {
}
"""
        matches = extract_help(text)
        names = [m[0] for m in matches]
        self.assertEqual(names, ["cmd_a", "cmd_b"])

    def test_no_proc_not_counted(self):
        """define_cmd_args without a matching proc should not be counted."""
        text = """
sta::define_cmd_args "orphan_cmd" {[-opt1]}
"""
        matches = extract_help(text)
        self.assertEqual(len(matches), 0)

    def test_empty_args(self):
        text = """
sta::define_cmd_args "no_args_cmd" {}
proc no_args_cmd { } {
}
"""
        matches = extract_help(text)
        self.assertEqual(len(matches), 1)
        self.assertEqual(matches[0][0], "no_args_cmd")

    def test_checker_off_no_spaces(self):
        text = """
sta::define_cmd_args "hidden" {[-opt]} ;#checker off
proc hidden { args } {
}
"""
        matches = extract_help(text)
        self.assertEqual(len(matches), 0)

    def test_checker_off_on_multiline_closing_brace(self):
        """checker off on the closing } line of a multi-line block."""
        text = """
sta::define_cmd_args "hidden" {[-opt1] \\
                               [-opt2]
} ;#checker off
proc hidden { args } {
}
"""
        matches = extract_help(text)
        self.assertEqual(len(matches), 0)


class TestExtractProc(unittest.TestCase):
    """Tests for extract_proc() which parses sta::parse_key_args blocks."""

    def test_basic_proc(self):
        text = """
proc my_command { args } {
  sta::parse_key_args "my_command" args \\
    keys {-opt1 -opt2} \\
    flags {-verbose}
}
"""
        matches = extract_proc(text)
        self.assertEqual(len(matches), 1)
        self.assertEqual(matches[0][0], "my_command")

    def test_checker_off_excluded(self):
        text = """
proc hidden { args } {
  sta::parse_key_args "hidden" args \\
    keys {-opt1} \\
    flags {-verbose} ;# checker off
}
"""
        matches = extract_proc(text)
        self.assertEqual(len(matches), 0)


if __name__ == "__main__":
    unittest.main()
