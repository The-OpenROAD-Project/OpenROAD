# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


class TestManTclEntryPoint(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        scripts = self.root / "docs/src/scripts"
        scripts.mkdir(parents=True)
        for filename in ("man_tcl_check.py", "extract_utils.py"):
            shutil.copyfile(Path(__file__).with_name(filename), scripts / filename)
        self.checker = scripts / "man_tcl_check.py"

        for module in ("selected", "unrelated"):
            module_dir = self.root / "openroad" / module
            (module_dir / "src").mkdir(parents=True)
            (module_dir / "src/commands.tcl").write_text(
                'sta::define_cmd_args "example" {}\n'
                "proc example { args } {\n"
                '  sta::parse_key_args "example" args keys {} flags {}\n'
                "}\n",
                encoding="utf-8",
            )
            (module_dir / "README.md").write_text(
                (
                    "### Example\n\n```tcl\nexample\n```\n"
                    if module == "selected"
                    else "# Missing command documentation\n"
                ),
                encoding="utf-8",
            )
        self.cwd = self.root / "elsewhere"
        self.cwd.mkdir()

    def run_check(self, *args, main_module=False):
        entry = ["-m", "man_tcl_check"] if main_module else [str(self.checker)]
        pythonpath = str(self.checker.parent)
        if "PYTHONPATH" in os.environ:
            pythonpath = os.pathsep.join([pythonpath, os.environ["PYTHONPATH"]])
        return subprocess.run(
            [sys.executable, *entry, *args],
            cwd=self.cwd,
            env={**os.environ, "PYTHONPATH": pythonpath},
            capture_output=True,
            text=True,
            timeout=10,
        )

    def test_checks_only_the_requested_module(self):
        result = self.run_check("--module", "selected")
        self.assertEqual(result.returncode, 0, result.stderr)
        result = self.run_check("--module", "unrelated")
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertIn("help count (1) != readme count (0)", result.stderr)

    def test_module_entry_uses_runfiles_data(self):
        for filename in ("man_tcl_check.py", "extract_utils.py"):
            runfile = self.checker.with_name(filename)
            runfile.unlink()
            runfile.symlink_to(Path(__file__).with_name(filename).absolute())

        result = self.run_check("--module", "selected", main_module=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        (self.root / "openroad/selected/README.md").write_text(
            "# Missing command documentation\n", encoding="utf-8"
        )
        result = self.run_check("--module", "selected", main_module=True)
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertIn("help count (1) != readme count (0)", result.stderr)

    def test_module_is_required(self):
        result = self.run_check()
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertIn("--module", result.stderr)

    def test_missing_readme_fails(self):
        (self.root / "openroad/selected/README.md").unlink()
        result = self.run_check("--module", "selected")
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertIn("No README found for module selected", result.stderr)

    def test_missing_tcl_sources_fails(self):
        (self.root / "openroad/selected/src/commands.tcl").unlink()
        result = self.run_check("--module", "selected")
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertIn("No Tcl sources found for module selected", result.stderr)


if __name__ == "__main__":
    unittest.main()
