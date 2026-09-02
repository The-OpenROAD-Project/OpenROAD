# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2024-2026, The OpenROAD Authors

# dielectric epsilon lookup in UniversalFormat2FasterCap_923.py

import importlib.util
import os
import shutil
import subprocess
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
CONVERTER = os.path.join(HERE, "..", "rule_scripts", "UniversalFormat2FasterCap_923.py")

_spec = importlib.util.spec_from_file_location("converter_under_test", CONVERTER)
converter_module = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(converter_module)

# this script exists as three separate copies in the tree; run the
# behavioral checks below against all of them #
CONVERTERS = {
    "rule_scripts": CONVERTER,
    "calibration": os.path.join(
        HERE,
        "..",
        "calibration",
        "fasterCap",
        "scripts",
        "UniversalFormat2FasterCap_923.py",
    ),
    "fastercap_model": os.path.join(
        HERE, "rcx_v2", "FasterCapModel", "scripts", "UniversalFormat2FasterCap_923.py"
    ),
}

PROCESS_OUT = """\
CONDUCTOR {
	name M1
	height 0.1
	thickness 0.1
	min_width 0.1
	min_spacing 0.1
	top_extension 0
	bottom_extension 0
	resistivity 0.1
}
DIELECTRIC {
	name M1_diel
	epsilon 2.0
	non_conformal_metal
	thickness 1.0
	slope 0
	next_met 1
}
DIELECTRIC {
	name M2_diel
	epsilon 3.0
	non_conformal_metal
	thickness 1.0
	slope 0
	next_met 2
}
DIELECTRIC {
	name M3_diel
	epsilon 4.0
	non_conformal_metal
	thickness 1.0
	slope 0
	next_met 3
}
"""

# pattern window starting above the bottom of the process
WIRES_BUG1 = """\
PATTERN TYP/test_pattern
GROUND_PLANE 2 M2_gnd HEIGHT 1.0 1.1 THICKNESS 0.1
GROUND_PLANE 3 M3_gnd HEIGHT 2.9 3.0 THICKNESS 0.1
DIELECTRIC M2_diel HEIGHT 1.0 2.0 EPSILON 3.0
DIELECTRIC M3_diel HEIGHT 2.0 3.0 EPSILON 4.0
WIRE 1 M3_w1 LL 0.0 1.9 LR 1.0 1.9 UR 1.0 2.1 UL 0.0 2.1 LENGTH 1.0 VOLTAGE 1
WINDOW_BBOX  LL -1.0  0.0 UR  2.0  1.0 LENGTH  1.0
"""

# pattern whose only dielectric is the topmost layer in the process
WIRES_BUG2 = """\
PATTERN TYP/test_pattern_boundary
GROUND_PLANE 3 M3_gnd HEIGHT 2.0 2.1 THICKNESS 0.1
DIELECTRIC M3_diel HEIGHT 2.0 3.0 EPSILON 4.0
WIRE 1 M3_w1 LL 0.0 2.5 LR 1.0 2.5 UR 1.0 2.7 UL 0.0 2.7 LENGTH 1.0 VOLTAGE 1
WINDOW_BBOX  LL -1.0  0.0 UR  2.0  1.0 LENGTH  1.0
"""

# dielectric name absent from PROCESS_OUT
WIRES_UNKNOWN_DIEL = """\
PATTERN TYP/test_pattern_unknown
GROUND_PLANE 2 M2_gnd HEIGHT 1.0 1.1 THICKNESS 0.1
DIELECTRIC UnknownLayer_diel HEIGHT 1.0 2.0 EPSILON 3.0
WIRE 1 M2_w1 LL 0.0 1.0 LR 1.0 1.0 UR 1.0 1.2 UL 0.0 1.2 LENGTH 1.0 VOLTAGE 1
WINDOW_BBOX  LL -1.0  0.0 UR  2.0  1.0 LENGTH  1.0
"""

# a conductor at layer M0 so the dielectric fill-panel path also runs
# (it's gated on the pattern's lowest conductor layer); M2_diel has no
# conductor in its range, so it gets a synthetic fill panel referencing
# both its neighbors' epsilon
WIRES_DIEL_PANEL = """\
PATTERN TYP/test_diel_panel
GROUND_PLANE 0 M0_gnd HEIGHT 0.0 0.1 THICKNESS 0.1
DIELECTRIC M1_diel HEIGHT 0.0 1.0 EPSILON 2.0
DIELECTRIC M2_diel HEIGHT 1.0 2.0 EPSILON 3.0
DIELECTRIC M3_diel HEIGHT 2.0 3.0 EPSILON 4.0
WIRE 1 M3_w1 LL 0.0 2.0 LR 1.0 2.0 UR 1.0 2.2 UL 0.0 2.2 LENGTH 1.0 VOLTAGE 1
WINDOW_BBOX  LL -1.0  0.0 UR  2.0  1.0 LENGTH  1.0
"""


def run_converter(wires_text, pattern_name, converter=CONVERTER):
    workdir = tempfile.mkdtemp()
    try:
        pat_dir = os.path.join(workdir, "TYP", pattern_name)
        os.makedirs(pat_dir)
        with open(os.path.join(workdir, "TYP", "process.out"), "w") as f:
            f.write(PROCESS_OUT)
        with open(os.path.join(pat_dir, "wires"), "w") as f:
            f.write(wires_text)
        os.makedirs(os.path.join(workdir, "fcout"))
        result = subprocess.run(
            [
                "python3",
                converter,
                "TYP/process.out",
                f"TYP/{pattern_name}",
                "fcout",
                "standard",
                "-sim_window_ext",
                "-20",
                "-20",
                "-20",
                "20",
                "20",
                "20",
            ],
            cwd=workdir,
            capture_output=True,
            text=True,
        )
        lst_path = os.path.join(workdir, "fcout", pattern_name, "wires.lst")
        lst_text = None
        if os.path.exists(lst_path):
            with open(lst_path) as f:
                lst_text = f.read()
        return result.returncode, result.stderr, lst_text
    finally:
        shutil.rmtree(workdir, ignore_errors=True)


def epsilons_for(lst_text, wire_name):
    # panel lines are "C <file> <epsilon*1e-6> ..."
    values = []
    for line in lst_text.splitlines():
        if line.startswith("C ") and wire_name in line:
            values.append(float(line.split()[2]))
    return values


class TestDielectricEpsilon(unittest.TestCase):
    def test_shapeorder_uses_global_dielectric_index(self):
        for label, converter in CONVERTERS.items():
            with self.subTest(converter=label):
                rc, stderr, lst_text = run_converter(
                    WIRES_BUG1, "test_pattern", converter
                )
                self.assertEqual(rc, 0, stderr)
                self.assertIsNotNone(lst_text)
                values = epsilons_for(lst_text, "M3_w1")
                self.assertTrue(values)
                self.assertEqual(set(values), {4.0e-6})

    def test_no_indexerror_at_topmost_dielectric(self):
        for label, converter in CONVERTERS.items():
            with self.subTest(converter=label):
                rc, stderr, lst_text = run_converter(
                    WIRES_BUG2, "test_pattern_boundary", converter
                )
                self.assertEqual(rc, 0, stderr)
                self.assertIsNotNone(lst_text)
                # nothing is modeled above the topmost dielectric, so the
                # wire's top panel clamps to M3_diel's own epsilon #
                top_line = next(
                    l
                    for l in lst_text.splitlines()
                    if "wire_M3_w1" in l and "-top.txt" in l
                )
                self.assertEqual(float(top_line.split()[2]), 4.0e-6)

    def test_dielectric_fill_panel_uses_correct_neighbor_epsilon(self):
        for label, converter in CONVERTERS.items():
            with self.subTest(converter=label):
                rc, stderr, lst_text = run_converter(
                    WIRES_DIEL_PANEL, "test_diel_panel", converter
                )
                self.assertEqual(rc, 0, stderr)
                self.assertIsNotNone(lst_text)
                lines = lst_text.splitlines()
                top_line = next(
                    l for l in lines if "dielectric_M1_diel" in l and "-top.txt" in l
                )
                bottom_line = next(
                    l for l in lines if "dielectric_M2_diel" in l and "-bottom.txt" in l
                )
                # M1_diel's top borders M2_diel (shapeorder + 1); M2_diel's
                # bottom borders M1_diel (shapeorder - 1) #
                self.assertEqual(float(top_line.split()[2]), 3.0e-6)
                self.assertEqual(float(bottom_line.split()[2]), 2.0e-6)

    def test_unmatched_dielectric_name_raises(self):
        for label, converter in CONVERTERS.items():
            with self.subTest(converter=label):
                rc, stderr, _ = run_converter(
                    WIRES_UNKNOWN_DIEL, "test_pattern_unknown", converter
                )
                self.assertNotEqual(rc, 0)
                self.assertIn("not found in process file", stderr)

    def test_duplicate_dielectric_name_raises(self):
        converter_module.processDielectrics = [
            converter_module.Dielectrics("M1_diel", 1.0, 2.0),
            converter_module.Dielectrics("M1_diel", 1.0, 3.0),
        ]
        with self.assertRaises(ValueError):
            converter_module.TranslateUniversalFile("unused", "unused")

    def test_safe_diel_epsilon_clamps_both_ends(self):
        converter_module.processDielectrics = [
            converter_module.Dielectrics("M1_diel", 1.0, 2.0),
            converter_module.Dielectrics("M2_diel", 1.0, 3.0),
            converter_module.Dielectrics("M3_diel", 1.0, 4.0),
        ]
        self.assertEqual(converter_module.safeDielEpsilon(-1), 2.0)
        self.assertEqual(converter_module.safeDielEpsilon(3), 4.0)


if __name__ == "__main__":
    unittest.main()
