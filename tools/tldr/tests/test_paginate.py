# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr._paginate import flatten_arrays, iter_objects


class PaginateTest(unittest.TestCase):
    def test_iter_objects_concatenated_dicts(self) -> None:
        text = '{"a": 1}\n{"a": 2}\n{"a": 3}\n'
        self.assertEqual([o["a"] for o in iter_objects(text)], [1, 2, 3])

    def test_iter_objects_robust_to_double_newline_in_string(self) -> None:
        # A double newline inside a string value must NOT split the object.
        text = '{"body": "line1\\n\\nline2"}\n{"body": "ok"}\n'
        objs = list(iter_objects(text))
        self.assertEqual(len(objs), 2)
        self.assertEqual(objs[0]["body"], "line1\n\nline2")

    def test_flatten_arrays_concatenates(self) -> None:
        text = '[{"id": 1}, {"id": 2}][{"id": 3}]'
        ids = [r["id"] for r in flatten_arrays(text)]
        self.assertEqual(ids, [1, 2, 3])

    def test_flatten_arrays_handles_dict_root(self) -> None:
        text = '{"id": 99}'
        self.assertEqual([r["id"] for r in flatten_arrays(text)], [99])


if __name__ == "__main__":
    unittest.main()
