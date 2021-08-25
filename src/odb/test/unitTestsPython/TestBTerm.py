import odb
import helper
import odbUnitTest

class TestBTerm(odbUnitTest.TestCase):
    def setUp(self):
        self.db, self.lib = helper.createSimpleDB()
        blockName = '1LevelBlock'
        self.block = odb.dbBlock_create(self.db.getChip(), blockName)
        self.and2 = self.lib.findMaster('and2')
        self.inst = odb.dbInst.create(self.block, self.and2, "inst")
        self.iterm_a = self.inst.findITerm('a')
        self.n_a = odb.dbNet.create(self.block, 'na')
        self.n_b = odb.dbNet.create(self.block, 'nb')
        self.bterm_a = odb.dbBTerm.create(self.n_a, 'IN_a')
        
    def tearDown(self):
        self.db.destroy(self.db)
    def test_idle(self):
        self.assertEqual(self.bterm_a.getNet().getName(), 'na')
        self.assertEqual(self.n_a.getBTermCount(), 1)
        self.assertEqual(self.n_a.getBTerms()[0].getName(), 'IN_a')
        self.assertEqual(self.n_b.getBTermCount(), 0)
    def test_connect(self):
        self.bterm_a.connect(self.n_b)
        self.assertEqual(self.bterm_a.getNet().getName(), 'nb')
        self.assertEqual(self.n_a.getBTermCount(), 0)
        self.assertEqual(self.n_a.getBTerms(), [])
        self.assertEqual(self.n_b.getBTermCount(), 1)
        self.assertEqual(self.n_b.getBTerms()[0].getName(), 'IN_a')
    def test_disconnect(self):
        self.bterm_a.disconnect()
        self.assertIsNone(self.bterm_a.getNet())
        self.assertEqual(self.n_a.getBTermCount(), 0)
        self.assertEqual(self.n_a.getBTerms(), [])
    
if __name__=='__main__':
    odbUnitTest.main()
    