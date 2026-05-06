# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.publish import MalformedBodyError, upsert
from tldr.render_markdown import BEGIN, END

BLOCK1 = f"{BEGIN}\nfirst\n{END}\n"
BLOCK2 = f"{BEGIN}\nsecond\n{END}\n"


class PublishUpsertTest(unittest.TestCase):
    def test_first_run_appends(self) -> None:
        body = "## My PR\n\nDescription here.\n"
        out = upsert(body, BLOCK1)
        # Original body content must be preserved byte-exact.
        self.assertTrue(out.startswith(body))
        self.assertIn(BEGIN, out)
        self.assertIn(END, out)

    def test_second_run_replaces_in_place(self) -> None:
        first = upsert("hello\n", BLOCK1)
        second = upsert(first, BLOCK2)
        # "first" content must be gone, "second" present, and the content
        # outside the sentinels must be byte-exact.
        before = second.split(BEGIN)[0]
        self.assertEqual(before, first.split(BEGIN)[0])
        self.assertIn("second", second)
        self.assertNotIn("first", second.split(BEGIN)[1].split(END)[0])
        # Exactly one sentinel pair, always.
        self.assertEqual(second.count(BEGIN), 1)
        self.assertEqual(second.count(END), 1)

    def test_orphan_begin_raises(self) -> None:
        body = f"hello\n{BEGIN}\nbroken\n"
        with self.assertRaises(MalformedBodyError):
            upsert(body, BLOCK1)

    def test_double_begin_raises(self) -> None:
        body = f"{BEGIN}\nA\n{END}\n{BEGIN}\nB\n{END}\n"
        with self.assertRaises(MalformedBodyError):
            upsert(body, BLOCK1)

    def test_block_must_have_sentinels(self) -> None:
        with self.assertRaises(ValueError):
            upsert("any", "no sentinels here")


if __name__ == "__main__":
    unittest.main()
