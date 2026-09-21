# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

"""Tests for schema input typing and validation (Stage E).

Run with:  python3 -m unittest test_schema_validation
"""

import unittest

from schema_models import Class, Field, Iterator, Relation


class UnknownKeyRejectionTest(unittest.TestCase):
    def test_field_rejects_unknown_key(self):
        with self.assertRaises(ValueError) as ctx:
            Field.from_dict({"name": "x_", "type": "int", "typo": 1})
        self.assertIn("typo", str(ctx.exception))

    def test_class_rejects_unknown_key(self):
        with self.assertRaises(ValueError) as ctx:
            Class.from_dict({"name": "dbFoo", "bogus": True})
        self.assertIn("bogus", str(ctx.exception))

    def test_class_type_alias_is_accepted(self):
        # "type" is remapped to base_class and must not be flagged as unknown.
        klass = Class.from_dict({"name": "dbFoo", "type": "dbObject"})
        self.assertEqual(klass.base_class, "dbObject")

    def test_relation_rejects_unknown_key(self):
        with self.assertRaises(ValueError):
            Relation.from_dict(
                {"parent": "a", "child": "b", "type": "1_n", "tbl_name": "t_", "x": 1}
            )


class IteratorTypingTest(unittest.TestCase):
    def test_true_strings_become_bools(self):
        it = Iterator.from_dict(
            {
                "name": "dbFooItr",
                "parentObject": "dbFoo",
                "tableName": "foo_tbl",
                "reversible": "true",
                "orderReversed": "true",
                "sequential": 0,
            }
        )
        self.assertIs(it.reversible, True)
        self.assertIs(it.orderReversed, True)
        self.assertIs(it.customEnd, False)  # absent -> default

    def test_rejects_unknown_key(self):
        with self.assertRaises(ValueError):
            Iterator.from_dict(
                {"name": "dbFooItr", "parentObject": "dbFoo", "tableName": "t", "z": 1}
            )


class RelationTypingTest(unittest.TestCase):
    def test_defaults_and_fields(self):
        rel = Relation.from_dict(
            {"parent": "dbA", "child": "dbB", "type": "1_n", "tbl_name": "b_tbl_"}
        )
        self.assertEqual(rel.parent, "dbA")
        self.assertFalse(rel.hash)
        self.assertIsNone(rel.page_size)
        self.assertEqual(rel.flags, [])


if __name__ == "__main__":
    unittest.main()
