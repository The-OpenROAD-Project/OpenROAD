import odb
import helper
import odbUnitTest

class TestInst(odbUnitTest.TestCase):
    def setUp(self):
        self.db, self.lib = helper.createSimpleDB()
        self.block = helper.create2LevelBlock(self.db, self.lib, self.db.getChip())
        self.i1 = self.block.findInst('i1')
        
    def tearDown(self):
        self.db.destroy(self.db)
    def test_swap_master(self):
        self.assertEqual(self.i1.getMaster().getName(), 'and2')
        #testing with a gate with different mterm names
        gate = helper.createMaster2X1(self.lib, '_g2', 800, 800, '_a', '_b', '_o')
        self.assertFalse(self.i1.swapMaster(gate))
        self.assertNotEqual(self.i1.getMaster().getName(), '_g2')
        for iterm in self.i1.getITerms():
            self.assertNotIn(iterm.getMTerm().getName(), ['_a', '_b', '_o'])
        #testing with a gate with different mterms number
        gate = helper.createMaster3X1(self.lib, '_g3', 800, 800, '_a', '_b', '_c', '_o')
        self.assertFalse(self.i1.swapMaster(gate))
        self.assertNotEqual(self.i1.getMaster().getName(), '_g3')
        for iterm in self.i1.getITerms():
            self.assertNotIn(iterm.getMTerm().getName(), ['_a', '_b', '_c', '_o'])
        #testing with a gate with same mterm names
        gate = helper.createMaster2X1(self.lib, 'g2', 800, 800, 'a', 'b', 'o')
        self.assertTrue(self.i1.swapMaster(gate))
        self.assertEqual(self.i1.getMaster().getName(), 'g2')
        self.assertEqual(self.i1.getMaster().getWidth(), 800)
        self.assertEqual(self.i1.getMaster().getHeight(), 800)
        
if __name__=='__main__':
    odbUnitTest.main()
    