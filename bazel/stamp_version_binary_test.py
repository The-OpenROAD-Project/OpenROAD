# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
"""Stamps a real linked openroad and checks that -version reports the stamp.

Usage: stamp_version_binary_test.py <stamp_version> <openroad_unstamped>
  [--codesign]
"""

import os
import subprocess
import sys
import tempfile
import unittest

VERSION = "test-1-gabc"


class StampBinaryTest(unittest.TestCase):
    def version_of(self, binary):
        result = subprocess.run(
            [binary, "-version"], capture_output=True, text=True, check=True
        )
        return result.stdout.strip()

    def test_stamped_binary_reports_version(self):
        stamper, binary = ARGS[0], ARGS[1]
        self.assertEqual(self.version_of(binary), "bazel-nostamp")

        tmp = os.environ.get("TEST_TMPDIR") or tempfile.mkdtemp()
        status = os.path.join(tmp, "stable-status.txt")
        with open(status, "w", encoding="utf-8") as f:
            f.write(f"STABLE_GIT_VERSION {VERSION}\n")
        stamped = os.path.join(tmp, "openroad")
        subprocess.run([stamper, binary, status, stamped] + ARGS[2:], check=True)

        self.assertEqual(self.version_of(stamped), VERSION)


if __name__ == "__main__":
    ARGS = sys.argv[1:]
    unittest.main(argv=sys.argv[:1])
