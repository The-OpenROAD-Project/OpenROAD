# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Installs the openroad wheel with pip into an empty directory and imports
it in a fresh interpreter."""

import glob
import os
import subprocess
import sys
import tempfile
import unittest

# Run in a child interpreter with -I, so neither PYTHONPATH nor this test's
# runfiles can supply a module the wheel is missing.
IMPORT_CHECK = """
import sys
sys.path.insert(0, sys.argv[1])
import openroad
import odb, utl, gpl, ant, cgt, cts, dpl, drt, exa, fin, fsv, grt, ifp
import par, pdn, ppl, psm, rcx, rsz, stt, tap, wmk
from openroad import Design, Tech
tech = Tech()
assert odb.dbDatabase is not None
assert hasattr(ant, "AntennaChecker")
assert hasattr(wmk, "Watermark")
assert hasattr(fsv, "FieldSolver")
import rmp
assert hasattr(rmp, "Blif")
assert isinstance(utl.ANT, int)
print("imported", openroad.__file__)
"""


class WheelTest(unittest.TestCase):
    def test_install_and_import(self):
        wheels = glob.glob(os.path.join(os.path.dirname(__file__), "*.whl"))
        self.assertEqual(len(wheels), 1, wheels)
        with tempfile.TemporaryDirectory() as site:
            subprocess.run(
                [
                    sys.executable,
                    "-m",
                    "pip",
                    "install",
                    "--quiet",
                    "--no-index",
                    "--no-deps",
                    "--target",
                    site,
                    wheels[0],
                ],
                check=True,
            )
            result = subprocess.run(
                [sys.executable, "-I", "-c", IMPORT_CHECK, site],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn(os.path.join(site, "openroad"), result.stdout)


if __name__ == "__main__":
    unittest.main()
