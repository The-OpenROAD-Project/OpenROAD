import opendbpy as odb
import helper
import odbUnitTest
#destroying   dbInst,  dbNet,  dbBTerm,  dbBlock,  dbBPin,  dbWire,  dbCapNode,  dbCcSeg,  dbLib,  dbSWire,  dbObstruction,  dbRegion
#TODO         dbRSeg,  dbRCSeg,  dbRow,  dbTarget,  dbTech
class TestDestroy(odbUnitTest.TestCase):
    def setUp(self):
        self.db, lib = helper.createSimpleDB()
        self.parentBlock = odb.dbBlock_create(self.db.getChip(), 'Parent')
        self.block = helper.create2LevelBlock(self.db, lib, self.parentBlock)
        self.i1 = self.block.findInst('i1')
        self.i2 = self.block.findInst('i2')
        self.i3 = self.block.findInst('i3')
        self.n1 = self.i1.findITerm('a').getNet()
        self.n2 = self.i1.findITerm('b').getNet()
        self.n3 = self.i2.findITerm('a').getNet()
        self.n4 = self.i2.findITerm('b').getNet()
        self.n5 = self.i3.findITerm('a').getNet()
        self.n6 = self.i3.findITerm('b').getNet()
        self.n7 = self.i3.findITerm('o').getNet()
        
    def tearDown(self):
        self.db.destroy(self.db)
        
    def test_destroy_net(self):
        self.n1.destroy(self.n1)
        #check for Inst
        self.assertIsNone(self.i1.findITerm('a').getNet())
        #check for Iterms
        for iterm in self.block.getITerms():
            if(iterm.getNet() is None):
                self.assertEqual(iterm.getInst().getName(), 'i1')
                self.assertEqual(iterm.getMTerm().getName(), 'a')
            else:
                self.assertNotEqual(iterm.getNet().getName(), 'n1')
        #check for block and BTerms
        nets = self.block.getNets()
        for net in nets:
            self.assertNotEqual(net.getName(), 'n1')
        bterms = self.block.getBTerms()
        self.assertEqual(len(bterms), 4)
        for bterm in bterms:
            self.assertNotEqual(bterm.getName(), 'IN1')
            self.assertNotEqual(bterm.getNet().getName(), 'n1')
        self.assertIsNone(self.block.findBTerm('IN1'))
        self.assertIsNone(self.block.findNet('n1'))
    def test_destroy_inst(self):
        self.i1.destroy(self.i1)
        #check for block
        self.assertIsNone(self.block.findInst('i1'))
        for inst in self.block.getInsts():
            self.assertNotEqual(inst.getName(), 'i1')
        self.assertEqual(len(self.block.getITerms()), 6)
        #check for Iterms
        for iterm in self.block.getITerms():
            self.assertNotIn(iterm.getNet().getName(), ['n1', 'n2'])
            self.assertNotEqual(iterm.getInst().getName(), 'i1')
        #check for BTERMS
        IN1 = self.block.findBTerm('IN1')
        self.assertIsNone(IN1.getITerm())
        IN2 = self.block.findBTerm('IN2')
        self.assertIsNone(IN2.getITerm())
        #check for nets
        self.assertEqual(self.n1.getITermCount(), 0)
        self.assertEqual(self.n2.getITermCount(), 0)
        self.assertEqual(self.n5.getITermCount(), 1)
        self.assertNotEqual(self.n5.getITerms()[0].getInst().getName(), 'i1')
    def test_destroy_bterm(self):
        IN1 = self.block.findBTerm('IN1')
        IN1.destroy(IN1)
        #check for block and BTerms
        self.assertIsNone(self.block.findBTerm('IN1'))
        bterms = self.block.getBTerms()
        self.assertEqual(len(bterms), 4)
        for bterm in bterms:
            self.assertNotEqual(bterm.getName(), 'IN1')
        #check for n1
        self.assertEqual(self.n1.getBTermCount(), 0)
        self.assertEqual(self.n1.getBTerms(), [])
    def test_destroy_block(self):
        #creating a child block to parent block
        _block = helper.create1LevelBlock(self.db, self.db.getLibs()[0], self.parentBlock)
        self.assertEqual(len(self.parentBlock.getChildren()), 2)
        _block.destroy(_block)
        self.assertEqual(len(self.parentBlock.getChildren()), 1)
        odb.dbBlock_destroy(self.parentBlock)

        
    def test_destroy_bpin(self):
        IN1 = self.block.findBTerm('IN1')
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
        ccseg = odb.dbCCSeg_create(node1, node2)
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
        swire = odb.dbSWire_create(self.n4, 'ROUTED')
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
        parentRegion = odb.dbRegion_create(self.block, 'parentRegion')
        childRegion = odb.dbRegion_create(parentRegion, 'childRegion')
        childRegion.addInst(self.i1)
        return parentRegion, childRegion
    def test_create_regions(self):
        parentRegion, childRegion = self.setup_regions()
        self.assertEqual(self.i1.getRegion().getName(), childRegion.getName())
        self.assertEqual(len(parentRegion.getChildren()), 1)
        self.assertEqual(len(childRegion.getChildren()), 0)
        self.assertEqual(parentRegion.getChildren()[0].getName(), childRegion.getName())
        self.assertEqual(len(self.block.getRegions()), 2)
        self.assertEqual(self.i1.getRegion().getName(), childRegion.getName())
        self.assertEqual(childRegion.getParent().getName(), parentRegion.getName())
        self.assertIsNone(parentRegion.getParent())
    def test_destroy_region_child(self):
        parentRegion, childRegion = self.setup_regions()
        childRegion.destroy(childRegion)
        self.assertIsNone(self.i1.getRegion())
        self.assertEqual(len(parentRegion.getChildren()), 0)
        self.assertEqual(len(self.block.getRegions()), 1)
        self.assertEqual(self.block.getRegions()[0].getName(), parentRegion.getName())
        
    def test_destroy_region_parent(self):
        parentRegion, childRegion = self.setup_regions()
        parentRegion.destroy(parentRegion)
        self.assertEqual(len(self.block.getRegions()), 0)
    
    def test_destroy_trackgrid(self):
        tech = self.db.getLibs()[0].getTech()
        L1 = tech.findLayer("L1")
        grid = odb.dbTrackGrid_create(self.block,L1)
        self.assertIsNone(odb.dbTrackGrid_create(self.block,L1))
        grid.destroy(grid)
        self.assertIsNotNone(odb.dbTrackGrid_create(self.block,L1))

if __name__=='__main__':
    odbUnitTest.mainParallel(TestDestroy)
#     odbUnitTest.main()
    
