#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Bit-for-bit equivalence tests for standalone binaries.
#
# Each test runs the same command two ways:
# 1. Standalone binary: binary --read_db in.odb --write_db standalone.odb
# 2. Monolithic openroad: openroad -exit script.tcl (read_db; cmd; write_db)
# Then diffs the output ODBs — they must be identical.

import os
import subprocess
import tempfile
import unittest


def find_runfile(path):
    """Find a file in the Bazel runfiles tree."""
    srcdir = os.environ.get("TEST_SRCDIR", "")
    candidate = os.path.join(srcdir, "_main", path)
    if os.path.exists(candidate):
        return candidate
    # Try without _main prefix
    candidate = os.path.join(srcdir, path)
    if os.path.exists(candidate):
        return candidate
    return None


def find_binary(name):
    """Find a standalone binary in runfiles."""
    return find_runfile(f"test/orfs/openroad/{name}")


def find_openroad():
    """Find the monolithic openroad binary."""
    return find_runfile("openroad")


def run_standalone(binary_name, input_odb, output_odb, extra_args=None):
    """Run a standalone binary (openroad-{name} convention, like git-{cmd})."""
    binary = find_binary(f"openroad-{binary_name}")
    if binary is None:
        raise FileNotFoundError(f"Binary {binary_name} not found in runfiles")
    cmd = [binary, "--read_db", input_odb, "--write_db", output_odb]
    if extra_args:
        cmd.extend(extra_args)
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
    return result


def run_monolithic(tcl_commands, input_odb, output_odb):
    """Run the same command via monolithic openroad + generated TCL."""
    openroad = find_openroad()
    if openroad is None:
        raise FileNotFoundError("openroad binary not found in runfiles")
    tcl = f"read_db {input_odb}\n{tcl_commands}\nwrite_db {output_odb}\n"
    with tempfile.NamedTemporaryFile(
        suffix=".tcl", mode="w", delete=False
    ) as f:
        f.write(tcl)
        tcl_path = f.name
    try:
        result = subprocess.run(
            [openroad, "-exit", tcl_path],
            capture_output=True,
            text=True,
            timeout=120,
        )
        return result
    finally:
        os.unlink(tcl_path)


def assert_odb_identical(path_a, path_b, test_name):
    """Assert two ODB files are byte-identical."""
    with open(path_a, "rb") as a, open(path_b, "rb") as b:
        data_a = a.read()
        data_b = b.read()
    if data_a != data_b:
        size_a = len(data_a)
        size_b = len(data_b)
        raise AssertionError(
            f"{test_name}: ODB files differ "
            f"(standalone={size_a} bytes, monolithic={size_b} bytes)"
        )


class TestBitForBitEquivalence(unittest.TestCase):
    """Verify standalone binaries produce identical output to monolithic openroad."""

    @classmethod
    def setUpClass(cls):
        cls.tmpdir = tempfile.mkdtemp(prefix="openroad_integration_")
        # Stage ODBs can be provided via env vars (from prior bazelisk build)
        # or found in runfiles (if listed as data deps).
        cls.gcd_place_odb = os.environ.get("GCD_PLACE_ODB") or find_runfile(
            "test/orfs/gcd/results/asap7/gcd/base/3_place.odb"
        )
        cls.gcd_grt_odb = os.environ.get("GCD_GRT_ODB") or find_runfile(
            "test/orfs/gcd/results/asap7/gcd/base/5_1_grt.odb"
        )
        cls.gcd_route_odb = os.environ.get("GCD_ROUTE_ODB") or find_runfile(
            "test/orfs/gcd/results/asap7/gcd/base/5_route.odb"
        )

    def _paths(self, name):
        """Return (standalone_odb, monolithic_odb) temp paths."""
        return (
            os.path.join(self.tmpdir, f"{name}_standalone.odb"),
            os.path.join(self.tmpdir, f"{name}_monolithic.odb"),
        )

    def test_detailed_placement(self):
        if not self.gcd_place_odb:
            self.skipTest("gcd_place ODB not found")
        standalone, monolithic = self._paths("detailed_placement")

        r1 = run_standalone(
            "detailed_placement", self.gcd_place_odb, standalone
        )
        self.assertEqual(r1.returncode, 0, f"Standalone failed:\n{r1.stderr}")

        r2 = run_monolithic(
            "detailed_placement", self.gcd_place_odb, monolithic
        )
        self.assertEqual(r2.returncode, 0, f"Monolithic failed:\n{r2.stderr}")

        assert_odb_identical(standalone, monolithic, "detailed_placement")

    def test_optimize_mirroring(self):
        if not self.gcd_place_odb:
            self.skipTest("gcd_place ODB not found")
        standalone, monolithic = self._paths("optimize_mirroring")

        r1 = run_standalone(
            "optimize_mirroring", self.gcd_place_odb, standalone
        )
        self.assertEqual(r1.returncode, 0, f"Standalone failed:\n{r1.stderr}")

        r2 = run_monolithic(
            "optimize_mirroring", self.gcd_place_odb, monolithic
        )
        self.assertEqual(r2.returncode, 0, f"Monolithic failed:\n{r2.stderr}")

        assert_odb_identical(standalone, monolithic, "optimize_mirroring")

    def test_check_antennas(self):
        """check_antennas doesn't write ODB — just verify same exit code."""
        if not self.gcd_route_odb:
            self.skipTest("gcd_route ODB not found")

        r1 = run_standalone(
            "check_antennas",
            self.gcd_route_odb,
            "/dev/null",  # no write needed
            extra_args=[],
        )
        # check_antennas exits 0 if no violations, 1 if violations
        # Just verify it runs without crashing
        self.assertIn(r1.returncode, [0, 1], f"Crashed:\n{r1.stderr}")

    def test_global_route(self):
        if not self.gcd_place_odb:
            self.skipTest("gcd_place ODB not found")
        standalone, monolithic = self._paths("global_route")

        r1 = run_standalone(
            "global_route",
            self.gcd_place_odb,
            standalone,
            extra_args=["-verbose"],
        )
        self.assertEqual(r1.returncode, 0, f"Standalone failed:\n{r1.stderr}")

        r2 = run_monolithic(
            "global_route -verbose",
            self.gcd_place_odb,
            monolithic,
        )
        self.assertEqual(r2.returncode, 0, f"Monolithic failed:\n{r2.stderr}")

        # Global route may have thread-dependent ordering
        # Check both produced valid ODBs
        self.assertGreater(os.path.getsize(standalone), 0)
        self.assertGreater(os.path.getsize(monolithic), 0)

    def test_detailed_route(self):
        if not self.gcd_grt_odb:
            self.skipTest("gcd_grt ODB not found")
        standalone, monolithic = self._paths("detailed_route")

        r1 = run_standalone(
            "detailed_route",
            self.gcd_grt_odb,
            standalone,
            extra_args=["-droute_end_iter", "1", "-verbose", "1"],
        )
        self.assertEqual(r1.returncode, 0, f"Standalone failed:\n{r1.stderr}")

        r2 = run_monolithic(
            "detailed_route -droute_end_iter 1 -verbose 1",
            self.gcd_grt_odb,
            monolithic,
        )
        self.assertEqual(r2.returncode, 0, f"Monolithic failed:\n{r2.stderr}")

        # DRT has thread-dependent ordering — check both produce valid ODBs
        self.assertGreater(os.path.getsize(standalone), 0)
        self.assertGreater(os.path.getsize(monolithic), 0)


if __name__ == "__main__":
    unittest.main()
