import opendb as odb
import helper
import odbUnitTest

class TestITerm(odbUnitTest.TestCase):
    def setUp(self):
        self.db, self.lib = helper.createSimpleDB()
        blockName = '1LevelBlock'
        self.block = odb.dbBlock_create(self.db.getChip(), blockName)
        self.and2 = self.lib.findMaster('and2')
        self.inst = odb.dbInst.create(self.block, self.and2, "inst")
        self.iterm_a = self.inst.findITerm('a')
        
    def tearDown(self):
        self.db.destroy(self.db)
    def test_idle(self):
        self.assertIsNone(self.iterm_a.getNet())
    def test_connection_from_iterm(self):
        #Create net and Connect
        n = odb.dbNet_create(self.block, 'n1')
        self.assertEqual(n.getITermCount(), 0)
        self.assertEqual(n.getITerms(), [])
        self.iterm_a.connect(self.iterm_a, n)
        self.iterm_a.setConnected()
        self.assertEqual(self.iterm_a.getNet().getName(), 'n1')
        self.assertEqual(n.getITermCount(), 1)
        self.assertEqual(n.getITerms()[0].getMTerm().getName(), 'a')
        self.assertTrue(self.iterm_a.isConnected())
        #disconnect
        self.iterm_a.disconnect(self.iterm_a)
        self.iterm_a.clearConnected()
        self.assertEqual(n.getITermCount(), 0)
        self.assertEqual(n.getITerms(), [])
        self.assertIsNone(self.iterm_a.getNet())
        self.assertFalse(self.iterm_a.isConnected())
    def test_connection_from_inst(self):
        #Create net and Connect
        n = odb.dbNet_create(self.block, 'n1')
        self.assertEqual(n.getITermCount(), 0)
        self.assertEqual(n.getITerms(), [])
        self.iterm_a.connect(self.inst, n, self.inst.getMaster().findMTerm('a'))
        self.iterm_a.setConnected()
        self.assertEqual(self.iterm_a.getNet().getName(), 'n1')
        self.assertEqual(n.getITermCount(), 1)
        self.assertEqual(n.getITerms()[0].getMTerm().getName(), 'a')
        self.assertTrue(self.iterm_a.isConnected())
        #disconnect
        self.iterm_a.disconnect(self.iterm_a)
        self.iterm_a.clearConnected()
        self.assertEqual(n.getITermCount(), 0)
        self.assertEqual(n.getITerms(), [])
        self.assertIsNone(self.iterm_a.getNet())
        self.assertFalse(self.iterm_a.isConnected())
    # def test_avgxy_R0(self):
    #     x = odb.new_int(0)
    #     y = odb.new_int(0)
    #     self.assertFalse(self.iterm_a.getAvgXY(x, y))   #no mpin to work on
    #     mterm_a = self.and2.findMTerm('a')
    #     mpin_a = odb.dbMPin_create(mterm_a)
    #     self.assertFalse(self.iterm_a.getAvgXY(x, y))   #no boxes to work on
    #     geo_box_a_1 = odb.dbBox_create(mpin_a, self.lib.getTech().getLayers()[0], 0, 0, 50, 50)
    #     self.assertTrue(self.iterm_a.getAvgXY(x, y))
    #     self.assertEqual(odb.get_int(x), int((0+50)/2))
    #     self.assertEqual(odb.get_int(y), int((0+50)/2))
    #     geo_box_a_2 = odb.dbBox_create(mpin_a, self.lib.getTech().getLayers()[0], 5, 10, 100, 100)
    #     self.assertTrue(self.iterm_a.getAvgXY(x, y))
    #     self.assertEqual(odb.get_int(x), int( ((0+50)+(5+100))/4 ) )
    #     self.assertEqual(odb.get_int(y), int( ((0+50)+(10+100))/4 ) )
    # def test_avgxy_R90(self):
    #     x = odb.new_int(0)
    #     y = odb.new_int(0)
    #     mterm_a = self.and2.findMTerm('a')
    #     mpin_a = odb.dbMPin_create(mterm_a)
    #     geo_box_a_1 = odb.dbBox_create(mpin_a, self.lib.getTech().getLayers()[0], 0, 0, 50, 50)
    #     geo_box_a_2 = odb.dbBox_create(mpin_a, self.lib.getTech().getLayers()[0], 0, 0, 100, 100)
    #     self.inst.setOrient('R90')
    #     self.assertTrue(self.iterm_a.getAvgXY(x, y))
    #     self.assertEqual(odb.get_int(x), int( ((0+50)+(0+100))/4 )*-1 )
    #     self.assertEqual(odb.get_int(y), int( ((0+50)+(0+100))/4 ) )
if __name__=='__main__':
    odbUnitTest.mainParallel(TestITerm)
#     odbUnitTest.main()
    