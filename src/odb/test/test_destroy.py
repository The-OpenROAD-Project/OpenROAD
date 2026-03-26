import odb
import helper
import odbUnitTest
import unittest

# destroying   dbInst,  dbNet,  dbBTerm,  dbBlock,  dbBPin,  dbWire,  dbCapNode,
# dbCcSeg,  dbLib,  dbSWire,  dbObstruction,  dbRegion,  dbRSeg,  dbRow,  dbTech
# Note: dbRCSeg has no standalone Python API; dbTarget was removed from ODB


class TestDestroy(odbUnitTest.TestCase):
    def setUp(self):
        self.db, lib, self.design, self.ord_tech = helper.createSimpleDB()
        self.parentBlock = odb.dbBlock_create(self.db.getChip(), "Parent")
        self.block = helper.create2LevelBlock(self.db, lib, self.parentBlock)
        self.i1 = self.block.findInst("i1")
        self.i2 = self.block.findInst("i2")
        self.i3 = self.block.findInst("i3")
        self.n1 = self.i1.findITerm("a").getNet()
        self.n2 = self.i1.findITerm("b").getNet()
        self.n3 = self.i2.findITerm("a").getNet()
        self.n4 = self.i2.findITerm("b").getNet()
        self.n5 = self.i3.findITerm("a").getNet()
        self.n6 = self.i3.findITerm("b").getNet()
        self.n7 = self.i3.findITerm("o").getNet()

    def test_destroy_net(self):
        self.n1.destroy(self.n1)
        # check for Inst
        self.assertIsNone(self.i1.findITerm("a").getNet())
        # check for Iterms
        for iterm in self.block.getITerms():
            if iterm.getNet() is None:
                self.assertEqual(iterm.getInst().getName(), "i1")
                self.assertEqual(iterm.getMTerm().getName(), "a")
            else:
                self.assertNotEqual(iterm.getNet().getName(), "n1")
        # check for block and BTerms
        nets = self.block.getNets()
        for net in nets:
            self.assertNotEqual(net.getName(), "n1")
        bterms = self.block.getBTerms()
        self.assertEqual(len(bterms), 4)
        for bterm in bterms:
            self.assertNotEqual(bterm.getName(), "IN1")
            self.assertNotEqual(bterm.getNet().getName(), "n1")
        self.assertIsNone(self.block.findBTerm("IN1"))
        self.assertIsNone(self.block.findNet("n1"))

    def test_destroy_inst(self):
        self.i1.destroy(self.i1)
        # check for block
        self.assertIsNone(self.block.findInst("i1"))
        for inst in self.block.getInsts():
            self.assertNotEqual(inst.getName(), "i1")
        self.assertEqual(len(self.block.getITerms()), 6)
        # check for Iterms
        for iterm in self.block.getITerms():
            self.assertNotIn(iterm.getNet().getName(), ["n1", "n2"])
            self.assertNotEqual(iterm.getInst().getName(), "i1")
        # check for BTERMS
        IN1 = self.block.findBTerm("IN1")
        self.assertIsNone(IN1.getITerm())
        IN2 = self.block.findBTerm("IN2")
        self.assertIsNone(IN2.getITerm())
        # check for nets
        self.assertEqual(self.n1.getITermCount(), 0)
        self.assertEqual(self.n2.getITermCount(), 0)
        self.assertEqual(self.n5.getITermCount(), 1)
        self.assertNotEqual(self.n5.getITerms()[0].getInst().getName(), "i1")

    def test_destroy_bterm(self):
        IN1 = self.block.findBTerm("IN1")
        IN1.destroy(IN1)
        # check for block and BTerms
        self.assertIsNone(self.block.findBTerm("IN1"))
        bterms = self.block.getBTerms()
        self.assertEqual(len(bterms), 4)
        for bterm in bterms:
            self.assertNotEqual(bterm.getName(), "IN1")
        # check for n1
        self.assertEqual(self.n1.getBTermCount(), 0)
        self.assertEqual(self.n1.getBTerms(), [])

    def test_destroy_block(self):
        # creating a child block to parent block
        _block = helper.create1LevelBlock(
            self.db, self.db.getLibs()[0], self.parentBlock
        )
        self.assertEqual(len(self.parentBlock.getChildren()), 2)
        _block.destroy(_block)
        self.assertEqual(len(self.parentBlock.getChildren()), 1)
        odb.dbBlock_destroy(self.parentBlock)

    def test_destroy_bpin(self):
        IN1 = self.block.findBTerm("IN1")
        self.assertEqual(len(IN1.getBPins()), 1)
        P1 = IN1.getBPins()[0]
        P1.destroy(P1)
        self.assertEqual(len(IN1.getBPins()), 0)

    def test_create_destroy_wire(self):
        w = odb.dbWire.create(self.n7)
        self.assertIsNotNone(self.n7.getWire())
        w.destroy(w)
        self.assertIsNone(self.n7.getWire())

    def test_destroy_capnode(self):
        node2 = odb.dbCapNode_create(self.n2, 0, False)
        node1 = odb.dbCapNode_create(self.n1, 1, False)
        _ = odb.dbCCSeg_create(node1, node2)
        self.assertEqual(self.n1.getCapNodeCount(), 1)
        self.assertEqual(self.n1.getCcCount(), 1)
        node1.destroy(node1)
        self.assertEqual(self.n1.getCapNodeCount(), 0)
        self.assertEqual(self.n1.getCcCount(), 0)

    def test_destroy_ccseg(self):
        node2 = odb.dbCapNode_create(self.n2, 0, False)
        node1 = odb.dbCapNode_create(self.n1, 1, False)
        ccseg = odb.dbCCSeg_create(node1, node2)
        self.assertNotEqual(node1.getCCSegs(), [])
        self.assertNotEqual(self.block.getCCSegs(), [])
        self.assertEqual(self.n1.getCcCount(), 1)
        ccseg.destroy(ccseg)
        self.assertEqual(node1.getCCSegs(), [])
        self.assertEqual(self.block.getCCSegs(), [])
        self.assertEqual(self.n1.getCcCount(), 0)

    def test_destroy_lib(self):
        lib = self.db.getLibs()[0]
        lib.destroy(lib)
        self.assertEqual(self.db.getLibs(), [])

    def test_destroy_swire(self):
        swire = odb.dbSWire_create(self.n4, "ROUTED")
        self.assertNotEqual(self.n4.getSWires(), [])
        self.assertEqual(swire.getNet().getName(), self.n4.getName())
        swire.destroy(swire)
        self.assertEqual(self.n4.getSWires(), [])

    def test_destroy_obstruction(self):
        tech = self.db.getLibs()[0].getTech()
        L1 = tech.getLayers()[0]
        obst = odb.dbObstruction_create(self.block, L1, 0, 0, 1000, 1000)
        self.assertEqual(len(self.block.getObstructions()), 1)
        obst.destroy(obst)
        self.assertEqual(len(self.block.getObstructions()), 0)

    def setup_regions(self):
        parentRegion = odb.dbRegion_create(self.block, "parentRegion")
        return parentRegion

    def test_create_regions(self):
        parentRegion = self.setup_regions()
        self.assertIsNone(self.i1.getRegion())
        self.assertEqual(len(self.block.getRegions()), 1)
        self.assertEqual(self.block.getRegions()[0].getName(), parentRegion.getName())

    def test_destroy_region_parent(self):
        parentRegion = self.setup_regions()
        parentRegion.destroy(parentRegion)
        self.assertEqual(len(self.block.getRegions()), 0)

    def test_destroy_trackgrid(self):
        tech = self.db.getLibs()[0].getTech()
        L1 = tech.findLayer("L1")
        grid = odb.dbTrackGrid_create(self.block, L1)
        self.assertIsNone(odb.dbTrackGrid_create(self.block, L1))
        grid.destroy(grid)
        self.assertIsNotNone(odb.dbTrackGrid_create(self.block, L1))

    def test_create_destroy_row(self):
        lib = self.db.getLibs()[0]
        site = odb.dbSite.create(lib, "site1")
        self.assertIsNotNone(site)
        self.assertEqual(len(self.block.getRows()), 0)
        row = odb.dbRow.create(
            self.block, "row1", site, 0, 0, "R0", "HORIZONTAL", 10, 100
        )
        self.assertIsNotNone(row)
        self.assertEqual(row.getName(), "row1")
        self.assertEqual(len(self.block.getRows()), 1)
        row.destroy(row)
        self.assertEqual(len(self.block.getRows()), 0)

    def test_create_destroy_rseg(self):
        # getRSegs() skips the list head (zero rseg); use getZeroRSeg() instead.
        self.assertIsNone(self.n1.getZeroRSeg())
        rseg = odb.dbRSeg.create(self.n1, 0, 0, 0, False)
        self.assertIsNotNone(rseg)
        self.assertIsNotNone(self.n1.getZeroRSeg())
        # destroy() with explicit net: getNet() requires a target CapNode.
        rseg.destroy(rseg, self.n1)
        self.assertIsNone(self.n1.getZeroRSeg())

    def test_create_destroy_tech(self):
        tech2 = odb.dbTech.create(self.db, "tech2")
        self.assertIsNotNone(tech2)
        self.assertEqual(tech2.getName(), "tech2")
        self.assertEqual(len(self.db.getTechs()), 2)
        tech2.destroy(tech2)
        self.assertEqual(len(self.db.getTechs()), 1)


if __name__ == "__main__":
    unittest.main()
