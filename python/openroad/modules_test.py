# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""The per-tool modules keep separate namespaces while sharing one library,
and so one copy of the C++ state."""

import os
import sys
import unittest

import ant
import cts
import odb
import openroad
import wmk
from openroad import Design, Tech

import _openroad_loader

TEST_DIR = os.path.join(os.path.dirname(__file__), "..", "..", "src", "ant", "test")


class ModulesTest(unittest.TestCase):
    def test_every_module_loads_from_one_library(self):
        extensions = [
            module
            for name, module in list(sys.modules.items())
            if name in _openroad_loader.EXTENSIONS
        ]
        self.assertGreaterEqual(len(extensions), 6)
        self.assertEqual(
            {module.__file__ for module in extensions}, {_openroad_loader.LIBRARY}
        )

    @unittest.skipUnless(sys.platform.startswith("linux"), "reads /proc/self/maps")
    def test_library_mapped_once(self):
        with open("/proc/self/maps") as maps:
            libraries = {
                os.path.basename(line.split()[-1])
                for line in maps
                if line.rstrip().endswith(".so") and "openroad" in line.lower()
            }
        self.assertEqual(libraries, {"_openroad.so"})

    def test_finder_only_claims_openroad_modules(self):
        finder = next(f for f in sys.meta_path if type(f).__name__ == "_OpenRoadFinder")
        self.assertIsNotNone(finder.find_spec("src.ant._ant_py", None))
        self.assertIsNotNone(finder.find_spec("_ord_py", None))
        # Same final name in another package, or at the top level.
        self.assertIsNone(finder.find_spec("other._ant_py", None))
        self.assertIsNone(finder.find_spec("_ant_py", None))
        self.assertIsNone(finder.find_spec("other._ord_py", None))

    def test_tools_have_their_own_namespaces(self):
        self.assertTrue(hasattr(ant, "AntennaChecker"))
        self.assertFalse(hasattr(ant, "Tech"))
        self.assertFalse(hasattr(cts, "AntennaChecker"))
        self.assertFalse(hasattr(openroad, "AntennaChecker"))
        # Both tools define CtsOptions; each module has its own.
        self.assertIsNot(cts.CtsOptions, wmk.CtsOptions)

    def test_objects_cross_modules(self):
        tech = Tech()
        self.assertIsInstance(tech.getDB(), odb.dbDatabase)
        design = Design(tech)
        self.assertIsInstance(design.getAntennaChecker(), ant.AntennaChecker)

    def test_shared_state(self):
        tech = Tech()
        tech.readLef(os.path.join(TEST_DIR, "ant_check.lef"))
        design = Design(tech)
        design.readDef(os.path.join(TEST_DIR, "ant_check.def"))
        # ant's code checks the database that ord's code loaded.
        self.assertEqual(design.getAntennaChecker().checkAntennas(), 1)
        self.assertIsInstance(design.getBlock().findNet("net50"), odb.dbNet)


if __name__ == "__main__":
    unittest.main()
