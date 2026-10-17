# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
"""Unit tests for bazel/stamp_version.py."""

import os
import tempfile
import unittest

import stamp_version


def make_slot(version):
    return stamp_version.MARKER + version.encode().ljust(
        stamp_version.VERSION_SIZE, b"\0"
    )


class StampTest(unittest.TestCase):
    def test_overwrites_placeholder(self):
        data = b"head" + make_slot("bazel-nostamp") + b"tail"
        stamped = stamp_version.stamp(data, "26Q4-449-g6ffc67da54")
        self.assertEqual(stamped, b"head" + make_slot("26Q4-449-g6ffc67da54") + b"tail")
        self.assertEqual(len(stamped), len(data))

    def test_shorter_version_clears_old_text(self):
        data = make_slot("a-much-longer-placeholder")
        self.assertEqual(stamp_version.stamp(data, "26Q4"), make_slot("26Q4"))

    def test_missing_slot(self):
        with self.assertRaisesRegex(ValueError, "no version slot"):
            stamp_version.stamp(b"no slot here", "26Q4")

    def test_duplicate_slot(self):
        data = make_slot("x") + make_slot("y")
        with self.assertRaisesRegex(ValueError, "more than one"):
            stamp_version.stamp(data, "26Q4")

    def test_version_too_long(self):
        too_long = "v" * stamp_version.VERSION_SIZE
        with self.assertRaisesRegex(ValueError, "longer than"):
            stamp_version.stamp(make_slot("x"), too_long)


class ReadVersionTest(unittest.TestCase):
    def read(self, text):
        with tempfile.NamedTemporaryFile("w", delete=False) as f:
            f.write(text)
        try:
            return stamp_version.read_version(f.name)
        finally:
            os.unlink(f.name)

    def test_reads_git_version(self):
        status = "STABLE_GIT_COMMIT_TIME 1\nSTABLE_GIT_VERSION 26Q4-1-gabc\n"
        self.assertEqual(self.read(status), "26Q4-1-gabc")

    def test_missing_version_keeps_placeholder(self):
        self.assertEqual(self.read("STABLE_GIT_COMMIT_TIME 1\n"), "bazel-nostamp")


if __name__ == "__main__":
    unittest.main()
