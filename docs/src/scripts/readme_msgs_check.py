import os
import unittest

from md_roff_compat import man2_translate, man3_translate


class TestReadmeMsgsCheck(unittest.TestCase):
    """Test that README.md and messages.txt parse correctly into man page formats."""

    def setUp(self):
        test_dir = os.path.dirname(os.path.abspath(__file__))
        self.readme_path = os.path.join(test_dir, "../README.md")
        self.messages_path = os.path.join(test_dir, "../messages.txt")
        self.save_dir = os.path.join(os.getcwd(), "results/docs")
        os.makedirs(self.save_dir, exist_ok=True)

    def test_man2_translate(self):
        man2_translate(self.readme_path, self.save_dir)

    def test_man3_translate(self):
        man3_translate(self.messages_path, self.save_dir)


if __name__ == "__main__":
    unittest.main()
