import opendb as odb
import helper
import odbUnitTest
##################
##
## This test is intended for testing swig wrap
##
##################
class TestModule(odbUnitTest.TestCase):
    def setUp(self):
        self.db, self.lib = helper.createSimpleDB()
        self.block = helper.create1LevelBlock(self.db, self.lib, self.db.getChip())
        self.master_mod = odb.dbModule_create(self.block,"master_mod")
        self.parent_mod = odb.dbModule_create(self.block,"parent_mod")
        self.i1         = odb.dbModInst_create(self.parent_mod,self.master_mod,"i1")
        self.inst1      = self.block.findInst("inst")
    def tearDown(self):
        self.db.destroy(self.db)
    def test_default(self):
        self.check(self.master_mod,"getName","master_mod")
        self.assertEqual(self.block.findModule("parent_mod").getName(),"parent_mod")
        self.assertEqual(self.i1.getName(),"i1")
        self.assertEqual(self.i1.getParent().getName(),"parent_mod")
        self.assertEqual(self.i1.getMaster().getName(),"master_mod")
        self.assertEqual(self.master_mod.getModInst().getName(),"i1")
        self.assertEqual(self.parent_mod.getChildren()[0].getName(),"i1")
        self.assertEqual(self.block.getModInsts()[0].getName(),"i1")
        self.parent_mod.addInst(self.inst1)
        self.assertEqual(self.parent_mod.getInsts()[0].getName(),"inst")
        self.assertEqual(self.inst1.getModule().getName(),"parent_mod")
        self.parent_mod.removeInst(self.inst1)
        self.assertEqual(self.parent_mod.findModInst("i1").getName(),"i1")
        self.i1.destroy(self.i1)
        self.parent_mod.destroy(self.parent_mod)
if __name__=='__main__':
    odbUnitTest.mainParallel(TestModule)    