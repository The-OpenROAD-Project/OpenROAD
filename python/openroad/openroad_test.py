# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Python binding unit tests for the openroad package: one behavior per
test, each on its own Tech."""

import gc
import unittest

import odb
import openroad
from openroad import Design, Tech, set_thread_count, thread_count


class OpenRoadApiTest(unittest.TestCase):
    def test_thread_count(self):
        # The thread count belongs to the Tech's OpenRoad instance, which
        # must stay alive while it is used.
        tech = Tech()
        set_thread_count(2)
        self.assertEqual(thread_count(), 2)
        set_thread_count(1)
        self.assertEqual(thread_count(), 1)

    def test_thread_count_without_tech(self):
        # Destroying the current Tech leaves no OpenRoad instance.
        tech = Tech()
        del tech
        gc.collect()
        with self.assertRaisesRegex(RuntimeError, "ORD-0105"):
            set_thread_count(2)
        with self.assertRaisesRegex(RuntimeError, "ORD-0105"):
            thread_count()
        with self.assertRaisesRegex(RuntimeError, "ORD-0105"):
            openroad.get_db()

    def test_tech_has_database(self):
        tech = Tech()
        self.assertIsInstance(tech.getDB(), odb.dbDatabase)

    def test_design_without_block(self):
        tech = Tech()
        design = Design(tech)
        self.assertIsNone(design.getBlock())

    def test_several_techs_in_one_process(self):
        techs = [Tech() for _ in range(3)]
        # Compare the underlying C++ pointers, not the proxy objects.
        self.assertEqual(len({int(tech.getDB().this) for tech in techs}), 3)


if __name__ == "__main__":
    unittest.main()
