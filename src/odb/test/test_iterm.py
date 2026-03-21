import odb
import helper
import odbUnitTest
import unittest


class TestITerm(odbUnitTest.TestCase):
    def setUp(self):
        self.db, self.lib, self.design, self.ord_tech = helper.createSimpleDB()
        blockName = "1LevelBlock"
        self.block = odb.dbBlock_create(self.db.getChip(), blockName)
        self.and2 = self.lib.findMaster("and2")
        self.inst = odb.dbInst.create(self.block, self.and2, "inst")
        self.iterm_a = self.inst.findITerm("a")

    def test_idle(self):
        self.assertIsNone(self.iterm_a.getNet())

    def test_connection_from_iterm(self):
        # Create net and Connect
        n = odb.dbNet_create(self.block, "n1")
        self.assertEqual(n.getITermCount(), 0)
        self.assertEqual(n.getITerms(), [])
        self.iterm_a.connect(n)
        self.iterm_a.setConnected()
        self.assertEqual(self.iterm_a.getNet().getName(), "n1")
        self.assertEqual(n.getITermCount(), 1)
        self.assertEqual(n.getITerms()[0].getMTerm().getName(), "a")
        self.assertTrue(self.iterm_a.isConnected())
        # disconnect
        self.iterm_a.disconnect()
        self.iterm_a.clearConnected()
        self.assertEqual(n.getITermCount(), 0)
        self.assertEqual(n.getITerms(), [])
        self.assertIsNone(self.iterm_a.getNet())
        self.assertFalse(self.iterm_a.isConnected())

    def test_connection_from_inst(self):
        # Create net and Connect
        n = odb.dbNet_create(self.block, "n1")
        self.assertEqual(n.getITermCount(), 0)
        self.assertEqual(n.getITerms(), [])
        self.iterm_a.connect(n)
        self.iterm_a.setConnected()
        self.assertEqual(self.iterm_a.getNet().getName(), "n1")
        self.assertEqual(n.getITermCount(), 1)
        self.assertEqual(n.getITerms()[0].getMTerm().getName(), "a")
        self.assertTrue(self.iterm_a.isConnected())
        # disconnect
        self.iterm_a.disconnect()
        self.iterm_a.clearConnected()
        self.assertEqual(n.getITermCount(), 0)
        self.assertEqual(n.getITerms(), [])
        self.assertIsNone(self.iterm_a.getNet())
        self.assertFalse(self.iterm_a.isConnected())

    def test_equality(self):
        n = odb.dbNet_create(self.block, "n1")
        self.iterm_a.connect(n)

        iterm_via_find = self.inst.findITerm("a")
        iterm_via_net = n.getITerms()[0]

        self.assertEqual(iterm_via_find, self.iterm_a)
        self.assertEqual(iterm_via_net, self.iterm_a)
        self.assertEqual(iterm_via_find, iterm_via_net)
        self.assertFalse(iterm_via_find != self.iterm_a)
        self.assertFalse(iterm_via_net != self.iterm_a)
        self.assertEqual(hash(iterm_via_find), hash(self.iterm_a))
        self.assertEqual(hash(iterm_via_net), hash(self.iterm_a))
        iterm_set = {self.iterm_a, iterm_via_find, iterm_via_net}
        self.assertEqual(len(iterm_set), 1)

        # Different iterms must not compare equal.
        iterm_b = self.inst.findITerm("b")
        self.assertNotEqual(self.iterm_a, iterm_b)
        self.assertTrue(self.iterm_a != iterm_b)

    def test_avgxy_R0(self):
        result, x, y = self.iterm_a.getAvgXY()
        self.assertFalse(result)  # no mpin to work on
        mterm_a = self.and2.findMTerm("a")
        mpin_a = odb.dbMPin_create(mterm_a)
        result, x, y = self.iterm_a.getAvgXY()
        self.assertFalse(result)  # no boxes to work on
        geo_box_a_1 = odb.dbBox_create(
            mpin_a, self.lib.getTech().getLayers()[0], 0, 0, 50, 50
        )
        result, x, y = self.iterm_a.getAvgXY()
        self.assertTrue(result)
        self.assertEqual(x, int((0 + 50) / 2))
        self.assertEqual(y, int((0 + 50) / 2))
        geo_box_a_2 = odb.dbBox_create(
            mpin_a, self.lib.getTech().getLayers()[0], 5, 10, 100, 100
        )
        result, x, y = self.iterm_a.getAvgXY()
        self.assertTrue(result)
        self.assertEqual(x, int(((0 + 50) + (5 + 100)) / 4))
        self.assertEqual(y, int(((0 + 50) + (10 + 100)) / 4))

    def test_avgxy_R90(self):
        mterm_a = self.and2.findMTerm("a")
        mpin_a = odb.dbMPin_create(mterm_a)
        geo_box_a_1 = odb.dbBox_create(
            mpin_a, self.lib.getTech().getLayers()[0], 0, 0, 50, 50
        )
        geo_box_a_2 = odb.dbBox_create(
            mpin_a, self.lib.getTech().getLayers()[0], 0, 0, 100, 100
        )
        self.inst.setOrient("R90")
        result, x, y = self.iterm_a.getAvgXY()
        self.assertTrue(result)
        self.assertEqual(x, int(((0 + 50) + (0 + 100)) / 4) * -1)
        self.assertEqual(y, int(((0 + 50) + (0 + 100)) / 4))


if __name__ == "__main__":
    unittest.main()
