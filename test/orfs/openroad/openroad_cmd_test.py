#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

import os
import subprocess
import tempfile
import unittest

# Import the module under test
import openroad_cmd


class TestResolvePath(unittest.TestCase):
    """Test path resolution with BUILD_WORKING_DIRECTORY."""

    def test_absolute_path_unchanged(self):
        os.environ["BUILD_WORKING_DIRECTORY"] = "/home/user/project"
        self.assertEqual(openroad_cmd.resolve_path("/tmp/foo.odb"), "/tmp/foo.odb")

    def test_relative_path_resolved(self):
        os.environ["BUILD_WORKING_DIRECTORY"] = "/home/user/project"
        self.assertEqual(
            openroad_cmd.resolve_path("foo.odb"), "/home/user/project/foo.odb"
        )

    def test_relative_path_with_subdir(self):
        os.environ["BUILD_WORKING_DIRECTORY"] = "/home/user/project"
        self.assertEqual(
            openroad_cmd.resolve_path("results/out.odb"),
            "/home/user/project/results/out.odb",
        )

    def test_no_build_working_directory(self):
        os.environ.pop("BUILD_WORKING_DIRECTORY", None)
        result = openroad_cmd.resolve_path("foo.odb")
        self.assertEqual(result, os.path.join(os.getcwd(), "foo.odb"))


class TestParseIOArgs(unittest.TestCase):
    """Test I/O flag extraction and passthrough."""

    def setUp(self):
        os.environ["BUILD_WORKING_DIRECTORY"] = "/work"

    def test_read_db_extracted(self):
        io, flags = openroad_cmd.parse_io_args(["--read_db", "in.odb"])
        self.assertEqual(io["--read_db"], ["/work/in.odb"])
        self.assertEqual(flags, [])

    def test_command_flags_passthrough(self):
        io, flags = openroad_cmd.parse_io_args(
            ["--read_db", "in.odb", "-max_displacement", "10"]
        )
        self.assertEqual(flags, ["-max_displacement", "10"])

    def test_multiple_read_lef(self):
        io, flags = openroad_cmd.parse_io_args(
            ["--read_lef", "tech.lef", "--read_lef", "cells.lef", "--read_def", "d.def"]
        )
        self.assertEqual(io["--read_lef"], ["/work/tech.lef", "/work/cells.lef"])
        self.assertEqual(io["--read_def"], ["/work/d.def"])

    def test_write_db(self):
        io, flags = openroad_cmd.parse_io_args(
            ["--read_db", "/abs/in.odb", "--write_db", "out.odb"]
        )
        self.assertEqual(io["--read_db"], ["/abs/in.odb"])
        self.assertEqual(io["--write_db"], ["/work/out.odb"])


class TestGenerateTcl(unittest.TestCase):
    """Test TCL script generation."""

    def test_odb_mode(self):
        io = {"--read_db": ["/data/in.odb"], "--write_db": ["/data/out.odb"]}
        tcl = openroad_cmd.generate_tcl(
            "detailed_placement", io, ["-max_displacement", "10"]
        )
        self.assertIn("read_db /data/in.odb", tcl)
        self.assertIn("detailed_placement -max_displacement 10", tcl)
        self.assertIn("write_db /data/out.odb", tcl)

    def test_lef_def_mode(self):
        io = {
            "--read_lef": ["/data/tech.lef", "/data/cells.lef"],
            "--read_def": ["/data/placed.def"],
            "--write_def": ["/data/out.def"],
        }
        tcl = openroad_cmd.generate_tcl("detailed_placement", io, [])
        self.assertIn("read_lef /data/tech.lef", tcl)
        self.assertIn("read_lef /data/cells.lef", tcl)
        self.assertIn("read_def /data/placed.def", tcl)
        self.assertIn("detailed_placement", tcl)
        self.assertIn("write_def /data/out.def", tcl)

    def test_any_command_works(self):
        io = {"--read_db": ["/data/in.odb"]}
        tcl = openroad_cmd.generate_tcl(
            "global_route", io, ["-congestion_iterations", "50"]
        )
        self.assertIn("global_route -congestion_iterations 50", tcl)

    def test_no_flags(self):
        io = {"--read_db": ["/data/in.odb"]}
        tcl = openroad_cmd.generate_tcl("detailed_placement", io, [])
        self.assertIn("detailed_placement", tcl)


class TestDetailedPlacementBinary(unittest.TestCase):
    """Integration test: run the native cc_binary on real test data."""

    def _find_test_data(self):
        """Find LEF/DEF test data in runfiles."""
        runfiles = os.environ.get("TEST_SRCDIR", "")
        base = os.path.join(runfiles, "_main")
        lef = os.path.join(base, "src/dpl/test/Nangate45/Nangate45.lef")
        def_file = os.path.join(
            base, "src/dpl/test/nangate45-bench/gcd/gcd_replace.def"
        )
        if os.path.exists(lef) and os.path.exists(def_file):
            return lef, def_file
        return None, None

    def _find_binary(self):
        """Find the detailed_placement binary in runfiles."""
        runfiles = os.environ.get("TEST_SRCDIR", "")
        candidate = os.path.join(
            runfiles, "_main/test/orfs/openroad/detailed_placement"
        )
        if os.path.isfile(candidate):
            return candidate
        return None

    def test_detailed_placement_lef_def(self):
        """Run detailed_placement on gcd test data."""
        binary = self._find_binary()
        lef, def_file = self._find_test_data()
        if binary is None or lef is None:
            self.skipTest("Test data or binary not found in runfiles")

        with tempfile.NamedTemporaryFile(suffix=".def", delete=False) as f:
            out_def = f.name

        try:
            result = subprocess.run(
                [
                    binary,
                    "--read_lef",
                    lef,
                    "--read_def",
                    def_file,
                    "--write_def",
                    out_def,
                ],
                capture_output=True,
                text=True,
                timeout=30,
            )
            self.assertEqual(
                result.returncode,
                0,
                f"detailed_placement failed:\nstdout: {result.stdout}\nstderr: {result.stderr}",
            )
            self.assertTrue(os.path.exists(out_def), "Output DEF was not created")
            self.assertGreater(os.path.getsize(out_def), 0, "Output DEF is empty")
        finally:
            if os.path.exists(out_def):
                os.unlink(out_def)


if __name__ == "__main__":
    unittest.main()
