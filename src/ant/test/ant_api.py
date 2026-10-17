# Binding tests for the Python AntennaChecker API: arguments and results
# cross the binding. The C++ tests in cpp/ check the ratios themselves.

import os
import tempfile
import unittest

from openroad import Design, Tech

HERE = os.path.dirname(os.path.abspath(__file__))


class AntennaCheckerApiTest(unittest.TestCase):
    def setUp(self):
        self.tech = Tech()
        self.tech.readLef(os.path.join(HERE, "ant_check.lef"))
        self.design = Design(self.tech)
        self.design.readDef(os.path.join(HERE, "ant_check.def"))
        self.checker = self.design.getAntennaChecker()
        self.net50 = self.design.getBlock().findNet("net50")

    def test_check_antennas_counts(self):
        self.assertEqual(self.checker.checkAntennas(), 1)
        self.assertEqual(self.checker.antennaViolationCount(), 1)
        self.assertEqual(self.checker.pinViolationCount(), 2)

    def test_check_antennas_arguments(self):
        self.assertEqual(self.checker.checkAntennas(self.net50), 1)
        self.assertEqual(
            self.checker.checkAntennas(net=None, num_threads=2, verbose=True), 1
        )

    def test_check(self):
        design = self.checker.check()
        self.assertEqual(design.violatingNets(), 1)
        self.assertEqual(design.violatingPins(), 2)
        self.assertEqual([net.net.getName() for net in design.nets], ["net50"])
        self.assertEqual(len(self.checker.check(self.net50, 2).nets), 1)

    def test_check_net(self):
        net_check = self.checker.checkNet(self.net50)
        self.assertTrue(net_check.violated)
        self.assertEqual(net_check.net.getName(), "net50")
        self.assertEqual(
            [pin.gate.getInst().getName() for pin in net_check.pins],
            ["output50", "_264_"],
        )

    def test_unset_ratio_is_none(self):
        mcon, met1 = self.checker.checkNet(self.net50).pins[0].layers[:2]
        self.assertEqual(mcon.layer.getName(), "mcon")
        self.assertAlmostEqual(mcon.par.required, 3.0, places=2)
        self.assertIsNone(mcon.psr)
        self.assertAlmostEqual(met1.psr.actual, 275.2, places=1)
        self.assertTrue(met1.psr.violated)

    def test_ratio_margin_lowers_required(self):
        base = self.checker.checkNet(self.net50).pins[0].layers[0].par.required
        margin = self.checker.checkNet(self.net50, 50).pins[0].layers[0]
        self.assertAlmostEqual(margin.par.required, base * 0.5)

    def test_violations(self):
        self.checker.initAntennaRules()
        violations = self.checker.getAntennaViolations(self.net50, None, 0)
        self.assertGreater(len(violations), 0)
        self.assertGreater(violations[0].routing_level, 0)
        self.assertEqual(violations[0].diode_count_per_gate, 0)
        self.assertGreater(violations[0].excess_ratio, 1)

    def test_report_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            report = os.path.join(tmp, "ant.rpt")
            self.checker.setReportFileName(report)
            self.checker.checkAntennas()
            with open(report) as f:
                self.assertIn("Net: net50", f.read())


if __name__ == "__main__":
    unittest.main()
