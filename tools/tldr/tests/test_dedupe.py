# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import unittest

from tldr.dedupe import collapse, group_by_kind
from tldr.parsers.base import Finding, Severity


def _f(kind: str, key: str, headline: str) -> Finding:
    return Finding(
        parser="x",
        severity=Severity.actionable,
        kind=kind,
        headline=headline,
        dedupe_key=key,
    )


class DedupeTest(unittest.TestCase):
    def test_collapses_same_key_across_legs(self) -> None:
        # Same compile error reported on ubuntu22 + ubuntu24 should collapse.
        a = _f("compile_error", "compile_error:src/foo.cpp:42:1", "ubuntu22 hit")
        b = _f("compile_error", "compile_error:src/foo.cpp:42:1", "ubuntu24 hit")
        c = _f("compile_error", "compile_error:src/bar.cpp:7:3", "different")
        out = collapse([a, b, c])
        self.assertEqual(len(out), 2)
        self.assertEqual(out[0].headline, "ubuntu22 hit")  # first wins

    def test_group_by_kind_preserves_first_seen_order(self) -> None:
        a = _f("flow_test_fail", "k1", "h1")
        b = _f("metric_guard", "k2", "h2")
        c = _f("flow_test_fail", "k3", "h3")
        groups = group_by_kind([a, b, c])
        self.assertEqual(list(groups.keys()), ["flow_test_fail", "metric_guard"])
        self.assertEqual(len(groups["flow_test_fail"]), 2)


if __name__ == "__main__":
    unittest.main()
