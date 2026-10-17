import importlib.util
import sys
import types
import unittest
from pathlib import Path


class DiffSpefTest(unittest.TestCase):
    def test_filename_is_forwarded_to_diff_options(self):
        fake_rcx = types.ModuleType("rcx")
        fake_rcx.DiffOptions = type("DiffOptions", (), {})
        sys.modules["rcx"] = fake_rcx
        sys.modules["utl"] = types.ModuleType("utl")

        source = Path(__file__).with_name("rcx_aux.py")
        spec = importlib.util.spec_from_file_location("rcx_aux", source)
        rcx_aux = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(rcx_aux)

        class FakeOpenRCX:
            def diff_spef(self, options):
                self.options = options

        class FakeDesign:
            def __init__(self):
                self.open_rcx = FakeOpenRCX()

            def getOpenRCX(self):
                return self.open_rcx

        design = FakeDesign()
        rcx_aux.diff_spef(
            design,
            filename="roundtrip.spef",
            r_conn=True,
            r_res=True,
            r_cap=False,
            r_cc_cap=True,
        )

        options = design.open_rcx.options
        self.assertEqual(options.file, "roundtrip.spef")
        self.assertTrue(options.r_conn)
        self.assertTrue(options.r_res)
        self.assertFalse(options.r_cap)
        self.assertTrue(options.r_cc_cap)


if __name__ == "__main__":
    unittest.main()
